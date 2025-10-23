#ifndef __STRING_H__
#define __STRING_H__

#include <stddef.h> // For size_t

// Note: These are the functions you originally had in main.c
void* memcpy(void* restrict dest, const void* restrict src, size_t n);
void* memset(void* s, int c, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);
void* memmove(void* dest, const void* src, size_t n);

#endif // __STRING_H__
