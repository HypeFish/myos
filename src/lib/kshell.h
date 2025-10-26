#ifndef __KSHELL_H__
#define __KSHELL_H__

#include <stdint.h>

/**
 * @brief Initializes the kernel shell and prints the first prompt.
 */
void kshell_init(void);

/**
 * @brief Processes a single character from the keyboard.
 *
 * This function handles line buffering, backspace, and command execution
 * when 'Enter' is pressed.
 *
 * @param c The character to process.
 */
void kshell_process_char(char c);

#endif // __KSHELL_H__