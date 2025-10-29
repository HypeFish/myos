#ifndef __STRING_H__
#define __STRING_H__

#include <stddef.h> // For size_t

/**
 * @brief Copies n bytes from memory area src to memory area dest. The memory areas must not overlap.
 * @param dest Pointer to the destination memory area.
 * @param src Pointer to the source memory area.
 * @param n Number of bytes to copy.
 * @return Pointer to the destination memory area.
 */
void* memcpy(void* restrict dest, const void* restrict src, size_t n);

/**
 * @brief Sets the first n bytes of the block of memory pointed by s to the specified value (interpreted as an unsigned char).
 * @param s Pointer to the block of memory to fill.
 * @param c Value to be set. The value is passed as an int but is converted to unsigned char when set.
 * @param n Number of bytes to be set to the value.
 */
void* memset(void* s, int c, size_t n);

/**
 * @brief Compares the first n bytes of the memory areas s1 and s2.
 * @param s1 Pointer to the first memory area.
 * @param s2 Pointer to the second memory area.
 * @param n Number of bytes to compare.
 * @return An integer less than, equal to, or greater than zero if the first n bytes of s1 is found,
 *         respectively, to be less than, to match, or be greater than the first n bytes of s2.
 */
int memcmp(const void* s1, const void* s2, size_t n);

/**
 * @brief Compares two null-terminated strings lexicographically.
 * @param s1 Pointer to the first string.
 * @param s2 Pointer to the second string.
 * @return An integer less than, equal to, or greater than zero if s1 is found,
 *         respectively, to be less than, to match, or be greater than s2.
 */
void* memmove(void* dest, const void* src, size_t n);

/**
 * @brief Calculates the length of the null-terminated string s.
 * @param s Pointer to the string.
 * @return The number of characters in the string, excluding the null terminator.
 */
size_t strlen(const char* s);

/**
 * @brief Compares two null-terminated strings lexicographically.
 * @param s1 Pointer to the first string.
 * @param s2 Pointer to the second string.
 * @return An integer less than, equal to, or greater than zero if s1 is found,
 *         respectively, to be less than, to match, or be greater than s2.
 */
int strcmp(const char* s1, const char* s2);


#endif // __STRING_H__
