#include "vmm.h"
#include "pmm.h"
#include "serialport.h"
#include "string.h" // For memset
#include <stddef.h>     // For NULL

// We need 4KiB-aligned page tables
__attribute__((aligned(PAGE_SIZE)))
static page_table_t pml4;

__attribute__((aligned(PAGE_SIZE)))
static page_table_t pdpt; // For the identity map (0x0...)

__attribute__((aligned(PAGE_SIZE)))
static page_table_t pdpt_high; // For the higher-half map (0xffffffff...)

// We need 4 Page Directories to map 4GiB (4 * 1GiB)
// These will be SHARED by both the identity map and the kernel map.
__attribute__((aligned(PAGE_SIZE)))
static page_table_t pd[4];

// The "pd_high[2]" array has been DELETED.

void vmm_init(void) {
    // --- 1. Clear all tables ---
    memset(&pml4, 0, sizeof(page_table_t));
    memset(&pdpt, 0, sizeof(page_table_t));
    memset(&pdpt_high, 0, sizeof(page_table_t));
    memset(&pd, 0, sizeof(page_table_t) * 4);

    // --- Get physical addresses ---
    struct limine_kernel_address_response* kaddr = vmm_get_kernel_address();
    
    uint64_t pml4_phys = ((uint64_t)&pml4 - kaddr->virtual_base) + kaddr->physical_base;
    uint64_t pdpt_phys = ((uint64_t)&pdpt - kaddr->virtual_base) + kaddr->physical_base;
    uint64_t pdpt_high_phys = ((uint64_t)&pdpt_high - kaddr->virtual_base) + kaddr->physical_base;
    uint64_t pd_phys   = ((uint64_t)&pd   - kaddr->virtual_base) + kaddr->physical_base;

    
    // --- 2. IDENTITY MAP (for 0x0 - 0xFFFFFFFF) ---
    
    // Link PML4[0] -> PDPT
    pml4.entries[0] = pdpt_phys | PTE_PRESENT | PTE_WRITE | PTE_USER;

    // Link PDPT[0-3] -> PD[0-3]
    for (int i = 0; i < 4; i++) {
        pdpt.entries[i] = (pd_phys + (i * PAGE_SIZE)) | PTE_PRESENT | PTE_WRITE | PTE_USER;
    }

    // Fill the 4 PDs with 2MiB "Huge Page" entries for the first 4GiB
    uint64_t current_phys_addr = 0;
    for (int i = 0; i < 4; i++) { // For each PD
        for (int j = 0; j < 512; j++) { // For each entry in that PD
            pd[i].entries[j] = current_phys_addr | PTE_PRESENT | PTE_WRITE | PTE_HUGE_PAGE | PTE_USER;
            current_phys_addr += 0x200000; // Increment by 2MiB
        }
    }
    // This section is 100% CORRECT. It maps virt 0x0... to phys 0x0...


    // --- 3. KERNEL MAP (for 0xffffffff80000000 - ...) ---
    
    // Get the PML4 index for the kernel (0xffffffff...): 511
    uint64_t kernel_pml4_index = (kaddr->virtual_base >> 39) & 0x1FF;
    
    // Link PML4[511] -> PDPT_HIGH
    pml4.entries[kernel_pml4_index] = pdpt_high_phys | PTE_PRESENT | PTE_WRITE | PTE_USER;

    // Get the PDPT index for the kernel (0xffffffff80000000...): 510
    uint64_t kernel_pdpt_index = (kaddr->virtual_base >> 30) & 0x1FF;
    
    // --- THIS IS THE CRITICAL PART ---
    // We RE-USE the `pd` tables, which start mapping at `phys 0x0`.
    // This correctly maps `virt 0xffffffff80000000` to `phys 0x0`.
    
    // Link PDPT_HIGH[510] -> pd[0]
    pdpt_high.entries[kernel_pdpt_index + 0] = (pd_phys + 0 * PAGE_SIZE) | PTE_PRESENT | PTE_WRITE | PTE_USER;
    // Link PDPT_HIGH[511] -> pd[1]
    pdpt_high.entries[kernel_pdpt_index + 1] = (pd_phys + 1 * PAGE_SIZE) | PTE_PRESENT | PTE_WRITE | PTE_USER;

    
    // --- 4. Load the new Page Map ---
    __asm__ volatile ("mov %0, %%cr3" :: "r"(pml4_phys));
}