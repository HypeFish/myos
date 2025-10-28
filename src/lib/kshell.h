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

/**
 * @brief Prints an unsigned integer to the framebuffer.
 *
 * @param value The unsigned integer to print.
 */
void kshell_print_uint(uint64_t value);

/**
 * @brief Prints a hexadecimal representation of a value to the framebuffer.
 *
 * @param value The value to print in hexadecimal.
 */
void kshell_print_hex(uint64_t value);

#endif // __KSHELL_H__