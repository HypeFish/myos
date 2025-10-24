#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__

// This is just a basic implementation.
// You'll need to write serial_init() and serial_write_char()
// in your serialport.c based on the wiki.

void serial_init(void);
void serial_write_string(const char* s);
void serial_putchar(char c);

#endif // __SERIALPORT_H__