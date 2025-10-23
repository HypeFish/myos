#ifndef __PIC_H__
#define __PIC_H__

#include <stdint.h>

// PIC Port Definitions
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

// End-of-Interrupt Command
#define PIC_EOI      0x20

/**
 * @brief Remaps the PIC to use vectors 32-47 and enables
 * the Timer (IRQ 0) and Keyboard (IRQ 1).
 */
void pic_remap_and_init(void);

/**
 * @brief Sends the End-of-Interrupt (EOI) signal to the PIC(s).
 * @param irq The IRQ number (0-15) that was handled.
 */
void pic_send_eoi(uint8_t irq);

#endif // __PIC_H__
