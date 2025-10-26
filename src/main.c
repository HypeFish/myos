#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <serialport.h>
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "lib/string.h"
#include "framebuffer.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "pit.h"
#include "timer.h"

// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request
framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request
memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_kernel_address_request
kernel_address_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};


// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.
__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

// Halt and catch fire function
static void hcf(void) {
    __asm__ volatile ("cli");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

// Helper to print a hex address to the framebuffer
static void fb_print_hex(uint64_t n) {
    char hex_chars[] = "0123456789ABCDEF";
    fb_print("0x");
    for (int i = 60; i >= 0; i -= 4) {
        char c = hex_chars[(n >> i) & 0xF];
        fb_putchar(c);
    }
}

void shell_execute(const char* command) {
    if (strcmp(command, "help") == 0) {
        fb_print("Welcome to myOS!\n");
        fb_print("Available commands: help, clear, panic, alloc, ktest, uptime\n");
    }
    else if (strcmp(command, "clear") == 0) {
        fb_clear();
    }
    else if (strcmp(command, "panic") == 0) {
        fb_print("Triggering test panic (Divide by Zero).\n");
        // This will trigger your exception_handler
        volatile int zero = 0;
        volatile int panic = 1 / zero;
        (void)panic; // To avoid unused variable warning-
    }
    else if (strcmp(command, "alloc") == 0) {
        fb_print("Allocating one page...\n");
        void* p = pmm_alloc_page();
        if (p != NULL) {
            fb_print("  Successfully allocated 4KiB at: ");
            fb_print_hex((uint64_t)p);
            fb_print("\n  Freeing it back...\n");
            pmm_free_page(p);
        }
        else {
            fb_print("  Allocation failed! Out of memory.\n");
        }
    }
    else if (strcmp(command, "ktest") == 0) {
        fb_print("Testing kernel heap (kmalloc)...\n");

        fb_print("  Allocating 30 bytes (a1)...\n");
        void* a1 = kmalloc(30);
        fb_print("  Allocated at: "); fb_print_hex((uint64_t)a1); fb_print("\n");

        fb_print("  Allocating 500 bytes (a2)...\n");
        void* a2 = kmalloc(500);
        fb_print("  Allocated at: "); fb_print_hex((uint64_t)a2); fb_print("\n");

        fb_print("  Freeing a1...\n");
        kfree(a1);

        fb_print("  Allocating 30 bytes (a3)...\n");
        void* a3 = kmalloc(30);
        fb_print("  Allocated at: "); fb_print_hex((uint64_t)a3); fb_print("\n");

        fb_print("  Freeing a2 and a3...\n");
        kfree(a2);
        kfree(a3);

        fb_print("Heap test complete.\n");
    }
    else if (strcmp(command, "uptime") == 0) {
        uint64_t current_ticks = get_ticks();
        fb_print("Kernel ticks since boot: ");
        fb_print_hex(current_ticks);
        fb_print("\n");
        fb_print("(Note: At 100Hz, 0x64 ticks = 1 second)\n");
    }
    else if (strcmp(command, "") == 0) {
        // Do nothing on empty command
    }
    else {
        fb_print("Unknown command: ");
        fb_print(command);
        fb_print("\n");
    }
}

struct limine_framebuffer* global_framebuffer = NULL;

struct limine_memmap_response* vmm_get_memmap(void) {
    return memmap_request.response;
}

struct limine_kernel_address_response* vmm_get_kernel_address(void) {
    return kernel_address_request.response;
}
struct limine_framebuffer* vmm_get_framebuffer(void) {
    return framebuffer_request.response->framebuffers[0];
}

// The following will be our kernel's entry point.
void _start(void) {
    // --- 1. Init Serial (for debugging) ---
    serial_init();
    serial_write_string("Hello, Serial World!\n");

    // --- 2. Initialize core systems ---
    gdt_init();
    idt_init();
    pic_remap_and_init();
    serial_write_string("PIC remapped!\n");

    // --- 3. VALIDATE ALL LIMINE REQUESTS ---
    // This is the critical fix. We must check these *before* vmm_init.
    if (framebuffer_request.response == NULL
        || framebuffer_request.response->framebuffer_count < 1) {
        serial_write_string("ERROR: No framebuffer available.\n");
        hcf();
    }

    if (kernel_address_request.response == NULL) {
        serial_write_string("ERROR: Failed to get kernel address.\n");
        hcf();
    }

    if (memmap_request.response == NULL) {
        serial_write_string("ERROR: Failed to get memory map.\n");
        hcf();
    }

    // --- 4. Initialize Memory & Framebuffer ---
    global_framebuffer = framebuffer_request.response->framebuffers[0];

    // NOW it is safe to call vmm_init
    vmm_init(); // Virtual Memory Manager
    serial_write_string("VMM Phase 1b complete. New page map is active.\n");

    // NOW it is safe to call pmm_init
    pmm_init(memmap_request.response); // Physical Memory Manager

    heap_init(); // Kernel Heap
    fb_init(global_framebuffer);

    // --- 5. Enable Interrupts & Enter Idle Loop ---
    pit_init(100); // Initialize PIT to 100Hz

    serial_write_string("Interrupts enabled!\n");
    fb_print("Welcome to myOS!\n");
    fb_print("> ");

    for (;;) {
        __asm__ volatile ("sti; hlt");
    }
}