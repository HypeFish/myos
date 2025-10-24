#include "idt.h"
#include "serialport.h" // For debugging
#include <stddef.h>     // For NULL
#include "pic.h"        // For pic_send_eoi()
#include "io.h"         // For inb()
#include "framebuffer.h"  // For fb_putchar
#include "keyboard.h"     // For kbd_us_map
#include "string.h"       // For strcmp
#include "timer.h"

// --- NEW: Shell buffer ---
static char line_buffer[256];
static int buffer_index = 0;
#define MAX_BUFFER 255

static volatile uint64_t ticks = 0;

// --- NEW: Extern function (will live in main.c) ---
extern void shell_execute(const char *cmd);

uint64_t get_ticks(void) {
    return ticks;
}

// --- Define the IDT array (256 entries) ---
static struct InterruptDescriptor64 idt[256];

// --- Define the IDT descriptor ---
static struct idt_descriptor idt_desc;

// --- Extern declarations for our assembly ISR stubs (Exceptions 0-31) ---
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


// --- Extern declarations for our assembly IRQ stubs (Interrupts 32-47) ---
extern void* irq_stub_32;
extern void* irq_stub_33;
extern void* irq_stub_34;
extern void* irq_stub_35;
extern void* irq_stub_36;
extern void* irq_stub_37;
extern void* irq_stub_38;
extern void* irq_stub_39;
extern void* irq_stub_40;
extern void* irq_stub_41;
extern void* irq_stub_42;
extern void* irq_stub_43;
extern void* irq_stub_44;
extern void* irq_stub_45;
extern void* irq_stub_46;
extern void* irq_stub_47;

// Array of IRQ stub pointers
static void* irq_stubs[] = {
    &irq_stub_32, &irq_stub_33, &irq_stub_34, &irq_stub_35,
    &irq_stub_36, &irq_stub_37, &irq_stub_38, &irq_stub_39,
    &irq_stub_40, &irq_stub_41, &irq_stub_42, &irq_stub_43,
    &irq_stub_44, &irq_stub_45, &irq_stub_46, &irq_stub_47
};


// --- Helper function to set an IDT entry ---
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
struct registers {
    // Registers pushed by 'common_isr_stub' / 'common_irq_stub'
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    // Pushed by the individual stubs
    uint64_t int_no, err_code;
    // Pushed by the CPU automatically on interrupt
    uint64_t rip, cs, rflags, rsp, ss;
};

void __attribute__((used))exception_handler(struct registers* regs) {
    // (This function is unchanged)
    serial_write_string("Exception triggered: ");
    if (regs->int_no < 10) {
        char c[2] = { (char)(regs->int_no + '0'), '\0' };
        serial_write_string(c);
    } else if (regs->int_no < 32) {
        char c[3];
        c[0] = (char)((regs->int_no / 10) + '0');
        c[1] = (char)((regs->int_no % 10) + '0');
        c[2] = '\0';
        serial_write_string(c);
    }
    serial_write_string(" Error Code: ");
    char ec[2] = { (char)(regs->err_code + '0'), '\0' };
    serial_write_string(ec);
    serial_write_string("\n");
    serial_write_string("System Halted!\n");
    __asm__ volatile ("cli; hlt");
}


// --- C-level Hardware Interrupt Handler (IRQ) ---
void __attribute__((used))irq_handler(struct registers* regs) {
    uint8_t irq = regs->int_no - 32;

    switch (irq) {
        case 0: // Timer (IRQ 0)
        {
            ticks++;
            break;
        }
        
        case 1: // Keyboard (IRQ 1)
        {
            uint8_t scancode = inb(0x60);
            
            // Ignore key releases
            if (scancode >= 0x80) {
                break;
            }

            // Translate scancode
            char c = kbd_us_map[scancode];

            if (c == '\n') {
                // --- ENTER KEY ---
                fb_putchar('\n');
                line_buffer[buffer_index] = '\0'; // Null-terminate
                shell_execute(line_buffer);      // Process command
                buffer_index = 0;                  // Reset buffer
                fb_print("> ");                    // Print new prompt
            } else if (c == '\b') {
                // --- BACKSPACE KEY ---
                if (buffer_index > 0) {
                    buffer_index--;
                    fb_putchar('\b'); // fb_putchar handles the erase
                }
            } else if (c != 0) {
                // --- PRINTABLE CHAR ---
                if (buffer_index < MAX_BUFFER) {
                    line_buffer[buffer_index] = c;
                    buffer_index++;
                    fb_putchar(c); // Echo character to screen
                }
            }
            break;
        }
        // --- END OF CHANGE ---

        default:
            // (This is unchanged)
            serial_write_string("Unhandled IRQ: ");
            if (irq < 10) {
                char c[2] = { (char)(irq + '0'), '\0' };
                serial_write_string(c);
            } else {
                char c[3] = { (char)((irq / 10) + '0'), (char)((irq % 10) + '0'), '\0' };
                serial_write_string(c);
            }
            serial_write_string("\n");
            break;
    }

    // Send the End-of-Interrupt (EOI) signal to the PIC
    pic_send_eoi(irq);
}


// --- IDT Initialization Function ---
void idt_init(void) {
    // (This function is unchanged)
    serial_write_string("Initializing IDT...\n");

    idt_desc.limit = sizeof(idt) - 1;
    idt_desc.base = (uint64_t)&idt[0];

    uint8_t flags = 0x8E;

    // --- 1. Set up the exception handlers (vectors 0-31) ---
    for (uint8_t vector = 0; vector < 32; vector++) {
        if (isr_stubs[vector] != NULL) {
            idt_set_descriptor(vector, isr_stubs[vector], flags);
        }
    }

    // --- 2. Set up the hardware IRQ handlers (vectors 32-47) ---
    for (uint8_t vector = 0; vector < 16; vector++) {
        idt_set_descriptor(vector + 32, irq_stubs[vector], flags);
    }

    // --- 3. Set up a default handler for all other vectors (48-255) ---
    for (int vector = 48; vector < 256; vector++) {
        idt_set_descriptor(vector, &isr_stub_default, flags);
    }

    load_idt(&idt_desc);
    serial_write_string("IDT loaded!\n");
}