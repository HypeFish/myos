#include "pmm.h"
#include <limine.h>       // For the Limine requests
#include <serialport.h>   // For debugging output
#include "string.h"       // For memset

// We will be managing memory in 4KiB pages.
#define PAGE_SIZE 4096

// --- The Limine Memory Map Request ---
__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

// --- PMM State ---
static uint8_t* bitmap = NULL;
static uint64_t total_pages = 0;
static uint64_t highest_address = 0;
static uint64_t last_free_page = 0;
static uint64_t bitmap_size_in_bytes = 0;

// --- Bitmap Helper Functions ---

// Set a bit (page) in the bitmap to 1 (used)
static void bitmap_set(uint64_t page_index) {
    uint64_t byte_index = page_index / 8;
    uint8_t bit_index = page_index % 8;
    bitmap[byte_index] |= (1 << bit_index);
}

// Clear a bit (page) in the bitmap to 0 (free)
static void bitmap_clear(uint64_t page_index) {
    uint64_t byte_index = page_index / 8;
    uint8_t bit_index = page_index % 8;
    bitmap[byte_index] &= ~(1 << bit_index);
}

// Test if a bit (page) in the bitmap is 1 (used)
static int bitmap_test(uint64_t page_index) {
    uint64_t byte_index = page_index / 8;
    uint8_t bit_index = page_index % 8;
    return (bitmap[byte_index] & (1 << bit_index)) != 0;
}

// Helper function (you can remove this later)
static void serial_print_hex(uint64_t n) {
    char hex_chars[] = "0123456789ABCDEF";
    serial_write_string("0x");
    for (int i = 60; i >= 0; i -= 4) {
        char c = hex_chars[(n >> i) & 0xF];
        serial_putchar(c);
    }
}

void pmm_init(void) {
    serial_write_string("Initializing PMM...\n");

    struct limine_memmap_response *memmap_response = memmap_request.response;
    if (memmap_response == NULL) {
        serial_write_string("ERROR: No memory map from Limine.\n");
        return;
    }

    // --- 1. Loop 1: Find the highest memory address ---
    // We need this to know how many pages our bitmap needs to cover.
    for (uint64_t i = 0; i < memmap_response->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_response->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t top = entry->base + entry->length;
            if (top > highest_address) {
                highest_address = top;
            }
        }
    }

    total_pages = highest_address / PAGE_SIZE;
    // We need 1 bit per page. 8 bits per byte.
    bitmap_size_in_bytes = (total_pages / 8) + 1;

    serial_write_string("  Highest address: "); serial_print_hex(highest_address); serial_write_string("\n");
    serial_write_string("  Total pages: "); serial_print_hex(total_pages); serial_write_string("\n");
    serial_write_string("  Bitmap size: "); serial_print_hex(bitmap_size_in_bytes); serial_write_string(" bytes\n");

    // --- 2. Loop 2: Find a large enough [Usable] region to store the bitmap ---
    for (uint64_t i = 0; i < memmap_response->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_response->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size_in_bytes) {
            // We found a place for our bitmap!
            bitmap = (uint8_t*)entry->base;
            
            // Mark the entire bitmap as "used" (all 1s) by default
            memset(bitmap, 0xFF, bitmap_size_in_bytes);
            
            serial_write_string("  Bitmap placed at: "); serial_print_hex((uint64_t)bitmap); serial_write_string("\n");
            break;
        }
    }

    if (bitmap == NULL) {
        serial_write_string("ERROR: No suitable memory region found for bitmap!\n");
        return;
    }

    // --- 3. Loop 3: Mark all [Usable] pages as FREE (0) in the bitmap ---
    for (uint64_t i = 0; i < memmap_response->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_response->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            // Align base up to the nearest page, and top down to the nearest page
            uint64_t base_page = (entry->base + PAGE_SIZE - 1) / PAGE_SIZE;
            uint64_t top_page = (entry->base + entry->length) / PAGE_SIZE;

            if (top_page > base_page) {
                for (uint64_t j = base_page; j < top_page; j++) {
                    bitmap_clear(j); // Mark this page as FREE
                }
            }
        }
    }

    // --- 4. Mark the bitmap itself as USED ---
    uint64_t bitmap_pages = (bitmap_size_in_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
    uint64_t bitmap_base_page = (uint64_t)bitmap / PAGE_SIZE;
    for (uint64_t i = 0; i < bitmap_pages; i++) {
        bitmap_set(bitmap_base_page + i);
    }

    serial_write_string("PMM: Bitmap initialized. Free pages are now marked.\n");
}

/**
 * @brief Allocates a single 4KiB page of physical memory.
 */
void* pmm_alloc_page(void) {
    // Start searching from the last page we freed (or 0)
    for (uint64_t i = last_free_page; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            // Found a free page!
            bitmap_set(i);
            last_free_page = i + 1;
            return (void*)(i * PAGE_SIZE);
        }
    }

    // If we wrapped around, try searching from the beginning
    for (uint64_t i = 0; i < last_free_page; i++) {
        if (!bitmap_test(i)) {
            // Found a free page!
            bitmap_set(i);
            last_free_page = i + 1;
            return (void*)(i * PAGE_SIZE);
        }
    }

    // No free pages
    serial_write_string("ERROR: pmm_alloc_page() -> Out of memory!\n");
    return NULL;
}

/**
 * @brief Frees a previously allocated 4KiB physical page.
 */
void pmm_free_page(void* p) {
    if (p == NULL) return;

    uint64_t page_index = (uint64_t)p / PAGE_SIZE;
    if (page_index >= total_pages) {
        serial_write_string("ERROR: pmm_free_page() -> Invalid address\n");
        return;
    }

    if (!bitmap_test(page_index)) {
        serial_write_string("WARNING: pmm_free_page() -> Page was already free\n");
    }

    bitmap_clear(page_index);
    last_free_page = page_index;
}