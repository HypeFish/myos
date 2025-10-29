#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__

/**
 * @brief Initializes the serial port.
 */
void serial_init(void);

/**
 * @brief Writes a string to the serial port.
 * @param s The string to write.
 */
void serial_write_string(const char* s);

/**
 * @brief Writes a single character to the serial port.
 * @param c The character to write.
 */
void serial_putchar(char c);

#endif // __SERIALPORT_H__