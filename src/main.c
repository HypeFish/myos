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
#include "task.h"
#include "kshell.h"
#include "tar.h"

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

__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request
module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};


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
    // --- 1. Initialize Serial (for debugging errors) ---
    serial_init();

    // --- 2. Initialize Core CPU Systems ---
    gdt_init();
    idt_init();
    pic_remap_and_init();

    // --- 3. Validate Limine Bootloader Requests ---
    // Must be done before using any of the request responses.
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
    
    if (module_request.response == NULL || module_request.response->module_count < 1) {
        serial_write_string("ERROR: Failed to get initrd module.\n");
        hcf();
    }

    // --- 4. Initialize Memory Systems ---
    // (VMM must be first to set up the higher-half kernel)
    vmm_init(); 
    pmm_init(memmap_request.response);
    heap_init();

    // --- 5. Initialize Subsystems & Drivers ---
    global_framebuffer = framebuffer_request.response->framebuffers[0];
    fb_init(global_framebuffer);
    task_init();
    pit_init(100); // Initialize PIT to 100Hz
    
    // Load the initrd (RAM disk)
    struct limine_file* initrd = module_request.response->modules[0];
    tar_init(initrd->address);

    // --- 6. Print Welcome & Start Shell ---
    fb_print("Welcome to myOS! Type 'help' for a list of commands.\n");
    fb_print("Initrd loaded at: ");
    kshell_print_hex((uint64_t)initrd->address);
    fb_print("\n");
    kshell_init(); // Prints the first "> " prompt

    // --- 7. Enable Interrupts & Idle ---
    // All initialization is done. Interrupts can be enabled. The shell
    // will run in the timer interrupt context.
    __asm__ volatile ("sti");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}