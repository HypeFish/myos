#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>

/**
 * @brief Gets the current number of ticks since boot.
 * * @return volatile uint64_t The number of ticks.
 */
uint64_t get_ticks(void);

/**
 * @brief Timer tick handler called on each timer interrupt.
 */
void timer_tick(void);

#endif // __TIMER_H__