#ifndef __PIT_H__
#define __PIT_H__

#include <stdint.h>

/**
 * @brief Initializes the Programmable Interval Timer (PIT).
 * * Configures PIT Channel 0 to fire at the desired frequency.
 * * @param frequency The frequency in Hz.
 */
void pit_init(uint32_t frequency);

#endif // __PIT_H__