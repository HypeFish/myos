#include "gdt.h"        // Contains the correct 64-bit flag macros
#include "serialport.h" // For printing debug messages
#include <stdint.h>

// --- Define GDT Entries using CORRECT 64-bit flags from gdt.h ---

// Kernel Code Segment (64-bit)
#define GDT_KERNEL_CODE (GDT_ACCESS_PRESENT | GDT_ACCESS_PRIVL(0) | \
                         GDT_ACCESS_DESCTYPE(1) | GDT_ACCESS_EXECUTABLE | \
                         GDT_ACCESS_RW | GDT_GRAN_LONG_MODE | GDT_GRAN_4K_GRANULARITY)

// Kernel Data Segment (64-bit)
#define GDT_KERNEL_DATA (GDT_ACCESS_PRESENT | GDT_ACCESS_PRIVL(0) | \
                         GDT_ACCESS_DESCTYPE(1) | GDT_ACCESS_RW | \
                         GDT_GRAN_4K_GRANULARITY)

// --- GDT Array and Descriptor ---

// Our GDT array (Global Descriptor Table)
// It has 3 entries: Null, Kernel Code, Kernel Data
uint64_t gdt[3];

// The GDT descriptor (GDTR) structure used by the lgdt instruction
struct gdt_descriptor gdt_desc;


// --- GDT Initialization Function ---

// This function sets up the GDT and loads it.
void gdt_init(void) {
    serial_write_string("Initializing GDT...\n");

    // Fill the GDT array with the correct entries
    gdt[0] = 0;                      // Entry 0: Null Descriptor (required)
    gdt[1] = GDT_KERNEL_CODE;        // Entry 1: Kernel Code Segment (Selector 0x08)
    gdt[2] = GDT_KERNEL_DATA;        // Entry 2: Kernel Data Segment (Selector 0x10)

    // Prepare the GDT descriptor structure for the lgdt instruction
    gdt_desc.limit = sizeof(gdt) - 1; // Limit is size - 1
    gdt_desc.base = (uint64_t)&gdt[0]; // Base address of the GDT array

    // Load the GDT using the assembly function from gdt_asm.S
    load_gdt(&gdt_desc);

    // Reload segment registers using the assembly function from gdt_asm.S
    reload_segments();

    serial_write_string("GDT loaded!\n");
}