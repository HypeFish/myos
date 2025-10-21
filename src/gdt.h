#ifndef __GDT_H__
#define __GDT_H__

#include <stdint.h>

// GDT Descriptor (GDTR) structure
struct gdt_descriptor {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));


// --- Correct 64-bit GDT Entry Flags ---
// Access byte flags
#define GDT_ACCESS_PRESENT      (1ULL << 47) // Present bit
#define GDT_ACCESS_PRIVL(x)     (((uint64_t)(x) & 0x3) << 45)  // Privilege level (0-3)
#define GDT_ACCESS_DESCTYPE(x)  ((uint64_t)(x) << 44)  // Descriptor type (0=system, 1=code/data)
#define GDT_ACCESS_EXECUTABLE   (1ULL << 43) // Executable bit
#define GDT_ACCESS_RW           (1ULL << 41) // Readable/Writable bit

// Granularity flags
#define GDT_GRAN_LONG_MODE      (1ULL << 53) // Long mode (64-bit)
#define GDT_GRAN_4K_GRANULARITY (1ULL << 55) // 4K granularity


// Function to initialize the GDT
void gdt_init(void);

// Assembly functions
extern void load_gdt(struct gdt_descriptor* desc);
extern void reload_segments(void);

#endif // __GDT_H__

