#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <serialport.h>
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "string.h"
#include "framebuffer.h"

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

    // --- 3. Initialize Framebuffer ---
    if (framebuffer_request.response == NULL
        || framebuffer_request.response->framebuffer_count < 1) {
        serial_write_string("ERROR: No framebuffer available.\n");
        hcf();
    }
    
    // Get the first framebuffer
    struct limine_framebuffer* framebuffer = framebuffer_request.response->framebuffers[0];
    
    // Initialize our framebuffer console
    fb_init(framebuffer);
    
    // --- 4. Enable Interrupts ---
    sti();
    serial_write_string("Interrupts enabled!\n");

    // --- 5. Test our framebuffer! ---
    fb_print("Hello, Framebuffer World!\n");
    fb_print("This is a test of the framebuffer console.\n");
    fb_print("Kernel initialized successfully.\n");
    
    

    // --- 6. The idle loop ---
    serial_write_string("Kernel main loop reached. Idling...\n");
    for (;;) {
        __asm__ volatile ("sti; hlt");
    }
}

