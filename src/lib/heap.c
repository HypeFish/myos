#include "heap.h"
#include "pmm.h"
#include "serialport.h" // For serial_write_string
#include <stddef.h>
#include <stdbool.h>

// A header for each free memory block
typedef struct heap_block {
    size_t size;
    struct heap_block* next;
} heap_block_t;

// The start of our free list
static heap_block_t* free_list_head = NULL;

// Helper to print hex (you can move this to a common place later)
static void serial_print_hex(uint64_t n) {
    char hex_chars[] = "0123456789ABCDEF";
    serial_write_string("0x");
    for (int i = 60; i >= 0; i -= 4) {
        char c = hex_chars[(n >> i) & 0xF];
        serial_putchar(c);
    }
}

/**
 * @brief Requests more memory from the PMM to add to the heap.
 */
static void request_more_memory() {
    // Request one page for now
    void* p = pmm_alloc_page();
    if (p == NULL) {
        serial_write_string("ERROR: kmalloc failed to get new page from PMM\n");
        return;
    }
    
    // Treat this new page as one giant free block
    heap_block_t* new_block = (heap_block_t*)p;
    new_block->size = 4096 - sizeof(heap_block_t); // Size is page size minus our header
    new_block->next = NULL;

    // kfree will add it to the free list for us
    kfree((void*)(new_block + 1));
}

void heap_init(void) {
    free_list_head = NULL;
    serial_write_string("Kernel heap initialized.\n");
}

void* kmalloc(size_t size) {
    // We must align to the size of our header for sanity
    if (size < sizeof(heap_block_t)) {
        size = sizeof(heap_block_t);
    }

    // --- 1. Search the free list for a block ---
    heap_block_t* curr = free_list_head;
    heap_block_t* prev = NULL;
    
    while (curr != NULL) {
        if (curr->size >= size) {
            // Found a block!
            
            // For simplicity, we won't split the block.
            // (A "real" allocator would split it if it's much larger)
            
            if (prev == NULL) {
                // We are taking the head of the list
                free_list_head = curr->next;
            } else {
                // We are taking a block from the middle
                prev->next = curr->next;
            }
            
            curr->next = NULL; // Not on the free list anymore
            // serial_write_string("kmalloc: Found free block\n");
            
            // Return a pointer to the data area (just *after* the header)
            return (void*)(curr + 1);
        }
        prev = curr;
        curr = curr->next;
    }

    // --- 2. No block found. Request more memory from PMM. ---
    // serial_write_string("kmalloc: No block, requesting new page...\n");
    request_more_memory();
    
    // --- 3. Try again (recursive) ---
    // This is simple, but could stack overflow if PMM is out of memory.
    // A loop would be safer, but this is fine for now.
    return kmalloc(size);
}

void kfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    // Get the header, which is right before the data pointer
    heap_block_t* block = (heap_block_t*)ptr - 1;

    // Add it to the front of the free list
    block->next = free_list_head;
    free_list_head = block;
}