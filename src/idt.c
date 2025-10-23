#include "idt.h"        // Our new IDT header
#include "serialport.h" // For debugging
#include <stddef.h>     // For NULL

// --- Define the IDT array (256 entries) ---
// We make it 'static' so it's private to this file.
static struct InterruptDescriptor64 idt[256];

// --- Define the IDT descriptor ---
static struct idt_descriptor idt_desc;

// --- Extern declarations for our assembly ISR stubs ---
// We must tell C that these functions exist in another file (idt_asm.S)
extern void* isr_stub_0;
extern void* isr_stub_1;
extern void* isr_stub_2;
extern void* isr_stub_3;
extern void* isr_stub_4;
extern void* isr_stub_5;
extern void* isr_stub_6;
extern void* isr_stub_7;
extern void* isr_stub_8;
extern void* isr_stub_9;
extern void* isr_stub_10;
extern void* isr_stub_11;
extern void* isr_stub_12;
extern void* isr_stub_13;
extern void* isr_stub_14;
extern void* isr_stub_15;
extern void* isr_stub_16;
extern void* isr_stub_17;
extern void* isr_stub_18;
extern void* isr_stub_19;
extern void* isr_stub_20;
extern void* isr_stub_21;
// ... (stubs 22-27 are skipped)
extern void* isr_stub_28;
extern void* isr_stub_29;
extern void* isr_stub_30;
extern void* isr_stub_31;
extern void* isr_stub_default;

// Array of stub pointers to make initialization easier
static void* isr_stubs[] = {
    &isr_stub_0, &isr_stub_1, &isr_stub_2, &isr_stub_3, &isr_stub_4, &isr_stub_5,
    &isr_stub_6, &isr_stub_7, &isr_stub_8, &isr_stub_9, &isr_stub_10, &isr_stub_11,
    &isr_stub_12, &isr_stub_13, &isr_stub_14, &isr_stub_15, &isr_stub_16, &isr_stub_17,
    &isr_stub_18, &isr_stub_19, &isr_stub_20, &isr_stub_21,
    NULL, NULL, NULL, NULL, NULL, NULL, // 22-27 are reserved, set to NULL
    &isr_stub_28, &isr_stub_29, &isr_stub_30, &isr_stub_31
};

// --- Helper function to set an IDT entry ---
// This fills in the InterruptDescriptor64 struct.
void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    struct InterruptDescriptor64* descriptor = &idt[vector];
    uint64_t isr_address = (uint64_t)isr;

    descriptor->offset_1 = isr_address & 0xFFFF;
    descriptor->selector = 0x08; // Kernel Code Segment selector (from your GDT)
    descriptor->ist = 0;         // No IST for now
    descriptor->type_attributes = flags; // Gate type, DPL, Present bit
    descriptor->offset_2 = (isr_address >> 16) & 0xFFFF;
    descriptor->offset_3 = (isr_address >> 32) & 0xFFFFFFFF;
    descriptor->zero = 0;        // Reserved
}

// --- The C-level exception handler ---
// This is called by the common_isr_stub in idt_asm.S

// This struct must match the order of registers we pushed in idt_asm.S!
struct registers {
    // Registers pushed by 'common_isr_stub'
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    // Pushed by the individual stubs
    uint64_t int_no, err_code;
    // Pushed by the CPU automatically on interrupt
    uint64_t rip, cs, rflags, rsp, ss;
};

// The C handler function
// 'volatile' tells the compiler not to optimize this function away
void __attribute__((used)) exception_handler(struct registers* regs) {
    // For now, just print the interrupt number to the serial port
    serial_write_string("Exception triggered: ");

    // Simple int-to-char for debugging.
    // This is not a proper 'itoa', but works for 0-9.
    if (regs->int_no < 10) {
        char c[2] = { (char)(regs->int_no + '0'), '\0' };
        serial_write_string(c);
    }
    else if (regs->int_no < 32) {
        // Handle 10-31 (basic two digits)
        char c[3];
        c[0] = (char)((regs->int_no / 10) + '0');
        c[1] = (char)((regs->int_no % 10) + '0');
        c[2] = '\0';
        serial_write_string(c);
    }
    else {
        serial_write_string("(Unknown)");
    }

    serial_write_string(" Error Code: ");
    char c[2] = { (char)(regs->err_code + '0'), '\0' };
    serial_write_string(c);
    serial_write_string("\n");


    // In a real OS, you would panic here ("Blue Screen of Death")
    serial_write_string("System Halted!\n");
    __asm__ volatile ("cli; hlt"); // Clear interrupts and halt the system
}


// --- IDT Initialization Function ---
// This is the main function called from main.c
void idt_init(void) {
    serial_write_string("Initializing IDT...\n");

    // Prepare the IDT descriptor
    idt_desc.limit = sizeof(idt) - 1;       // Limit is size - 1
    idt_desc.base = (uint64_t)&idt[0];      // Base address of the IDT array

    // --- Set up the exception handlers (vectors 0-31) ---
    // 0x8E = 0b10001110
    //   P: 1 (Present)
    // DPL: 0 (Ring 0 - Kernel)
    //   S: 0 (System segment)
    //Type: E (64-bit Interrupt Gate)
    for (uint8_t vector = 0; vector < 32; vector++) {
        if (isr_stubs[vector] != NULL) {
            // Set the descriptor for this CPU exception
            idt_set_descriptor(vector, isr_stubs[vector], 0x8E);
        }
    }

    // --- Set up a default handler for all other vectors (32-255) ---
    // These are for hardware interrupts (PIC, APIC) or reserved
    for (int vector = 32; vector < 256; vector++) {
        idt_set_descriptor(vector, &isr_stub_default, 0x8E);
    }

    // Load the IDT using our assembly function
    load_idt(&idt_desc);

    serial_write_string("IDT loaded!\n");
}




