#ifndef __VMM_H__
#define __VMM_H__

#include "paging.h"
#include <limine.h> // <-- ADD THIS

// These functions will be implemented in main.c
struct limine_memmap_response* vmm_get_memmap(void);
struct limine_kernel_address_response* vmm_get_kernel_address(void);
struct limine_framebuffer* vmm_get_framebuffer(void);

/**
 * @brief Initializes the Virtual Memory Manager (VMM).
 * ... (rest of comment) ...
 */
void vmm_init(void);

/**
 * @brief Retrieves the kernel's PML4 page table.
 *
 * @return A pointer to the kernel's PML4 page table.
 */
page_table_t* vmm_get_kernel_pml4(void); // <-- ADD THIS LINE

#endif // __VMM_H__