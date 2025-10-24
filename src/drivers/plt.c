#include "pit.h"
#include "io.h"         // For outb()
#include "serialport.h" // For debugging

// PIT Registers
#define PIT_CHANNEL0_DATA 0x40
#define PIT_COMMAND_REG   0x43

// PIT's base frequency is ~1.193182 MHz
#define PIT_BASE_FREQUENCY 1193182

void pit_init(uint32_t frequency) {
    serial_write_string("Initializing PIT...\n");

    // 1. Calculate the divisor
    uint16_t divisor = PIT_BASE_FREQUENCY / frequency;
    if (PIT_BASE_FREQUENCY % frequency > frequency / 2) {
        divisor++; // Round up
    }
    
    // 2. Send the command byte
    // 0x36 = 0011 0110b
    // Channel 0
    // Access mode: Lobyte/Hibyte
    // Operating mode: Rate generator (Mode 2)
    // BCD/Binary mode: 16-bit binary
    outb(PIT_COMMAND_REG, 0x36);

    // 3. Send the divisor (low byte, then high byte)
    uint8_t low = divisor & 0xFF;
    uint8_t high = (divisor >> 8) & 0xFF;
    
    outb(PIT_CHANNEL0_DATA, low);
    outb(PIT_CHANNEL0_DATA, high);
    
    serial_write_string("PIT initialized at 100 Hz.\n");
}