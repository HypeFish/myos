#ifndef __PMM_H__
#define __PMM_H__

#include <stdint.h>

/**
 * @brief Initializes the Physical Memory Manager (PMM).
 * This function finds the available memory from the bootloader and
 * sets up the bitmap allocator.
 */
void pmm_init(void);

/**
 * @brief Allocates a single 4KiB page of physical memory.
 * @return The 64-bit physical address of the allocated page,
 * or 0 if no free pages are available.
 */
void* pmm_alloc_page(void);

/**
 * @brief Frees a previously allocated 4KiB physical page.
 * @param p The physical address of the page to free.
 */
void pmm_free_page(void* p);

#endif // __PMM_H__