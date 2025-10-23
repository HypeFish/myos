#include "pic.h"
#include "io.h" // For outb()

// Helper function for a short wait
static void io_wait(void) {
    outb(0x80, 0);
}

void pic_remap_and_init(void) {
    // --- Remap the PICs ---
    // We need to remap the PICs to a different offset (32-47)
    // because their default (8-15) conflicts with CPU exceptions.

    // 1. Start initialization (ICW1)
    outb(PIC1_COMMAND, 0x11);
    io_wait();
    outb(PIC2_COMMAND, 0x11);
    io_wait();

    // 2. Set vector offsets (ICW2)
    // PIC1 (Master) starts at vector 32 (0x20)
    outb(PIC1_DATA, 0x20);
    io_wait();
    // PIC2 (Slave) starts at vector 40 (0x28)
    outb(PIC2_DATA, 0x28);
    io_wait();

    // 3. Set up chaining (ICW3)
    // Tell Master it has a slave at IRQ 2 (0000 0100)
    outb(PIC1_DATA, 4);
    io_wait();
    // Tell Slave its cascade identity is 2
    outb(PIC2_DATA, 2);
    io_wait();

    // 4. Set 80x86 mode (ICW4)
    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

    // --- Mask interrupts ---
    // Start by masking all interrupts. We will unmask them one by one.
    // 0xFF = 1111 1111 (all masked)
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);

    // --- Unmask Timer and Keyboard ---
    // 0xFC = 1111 1100
    // Bit 0 (Timer) = 0 (unmasked)
    // Bit 1 (Keyboard) = 0 (unmasked)
    outb(PIC1_DATA, 0xFC);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        // If the IRQ came from the slave, send EOI to slave
        outb(PIC2_COMMAND, PIC_EOI);
    }
    // Always send EOI to master
    outb(PIC1_COMMAND, PIC_EOI);
}
