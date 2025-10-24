#ifndef __HEAP_H__
#define __HEAP_H__

#include <stddef.h> // For size_t

/**
 * @brief Initializes the kernel heap.
 * Must be called *after* pmm_init().
 */
void heap_init(void);

/**
 * @brief Allocates a block of memory of at least `size` bytes.
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
void* kmalloc(size_t size);

/**
 * @brief Frees a previously allocated block of memory.
 * @param ptr The pointer returned by kmalloc.
 */
void kfree(void* ptr);

#endif // __HEAP_H__