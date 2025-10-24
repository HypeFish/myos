#ifndef __IDT_H__
#define __IDT_H__

#include <stdint.h>

// --- MODIFICATION ---
// Moved the PACKED definition block to the top.
// We will undefine it at the very end of the file.
#ifdef _MSC_VER
#pragma pack(push,1)
#define PACKED
#else
#define PACKED __attribute__((packed))
#endif
// --- END MODIFICATION ---

// --- The IDT Entry Structure You Provided ---
struct InterruptDescriptor64 {
    uint16_t offset_1;        // offset bits 0..15
    uint16_t selector;        // a code segment selector in GDT or LDT
    uint8_t  ist;             // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
    uint8_t  type_attributes; // gate type, dpl, and p fields
    uint16_t offset_2;        // offset bits 16..31
    uint32_t offset_3;        // offset bits 32..63
    uint32_t zero;            // reserved
} PACKED;


// --- The IDT Descriptor (IDTR) Structure ---
// This is analogous to your gdt_descriptor
struct idt_descriptor {
    uint16_t limit;
    uint64_t base;
} PACKED; // <<< This will now work


// --- MODIFICATION ---
// Moved the pragma pop and undef to the end of the file.
#ifdef _MSC_VER
#pragma pack(pop)
#endif

#undef PACKED
// --- END MODIFICATION ---


// --- Public Function Prototypes ---

// The main function to set up the IDT
void idt_init(void);

// Assembly function (in idt_asm.S) to load the IDT Register (IDTR)
// This is just like your load_gdt() function
extern void load_idt(struct idt_descriptor* desc);

// --- Inline Assembly for STI/CLI ---

// sti (Set Interrupt Flag) - enables interrupts
static inline void sti(void) {
    __asm__ volatile ("sti");
}

// cli (Clear Interrupt Flag) - disables interrupts
static inline void cli(void) {
    __asm__ volatile ("cli");
}


#endif // __IDT_H__

