#include "vmm.h"
#include "pmm.h"
#include "serialport.h"
#include "string.h" // For memset
#include <stddef.h>     // For NULL
#include <limine.h>
// vmm.h includes paging.h, so VIRTUAL_MEMORY_OFFSET is available

// --- Page Tables ---
__attribute__((aligned(PAGE_SIZE)))
static page_table_t pml4;

__attribute__((aligned(PAGE_SIZE)))
static page_table_t pdpt; // For the identity map (0x0...)

__attribute__((aligned(PAGE_SIZE)))
static page_table_t pdpt_high; // For the higher-half kernel map (0xffffffff8...)

__attribute__((aligned(PAGE_SIZE)))
static page_table_t pdpt_hhdm; // For the HHDM (0xffff8...)

// --- Page Directories ---
// We need 32 PDs to map 32GiB for the identity/HHDM maps
__attribute__((aligned(PAGE_SIZE)))
static page_table_t pd[32];

// We need ONE PD for the kernel map (to map 1GiB)
__attribute__((aligned(PAGE_SIZE)))
static page_table_t pd_kernel;

// --- Page Tables (PTs) ---
// We need 512 PTs to map 1GiB (1 PD * 512 PTs)
// This is the only way to solve the 2MiB alignment bug.
__attribute__((aligned(PAGE_SIZE)))
static page_table_t pt_kernel[512];

/**
 * @brief Returns the virtual address of the kernel's PML4 page map.
 */
page_table_t* vmm_get_kernel_pml4(void) {
    // 'pml4' is the static variable at the top of this file
    return &pml4;
}


void vmm_init(void) {
    // --- 1. Clear all tables ---
    memset(&pml4, 0, sizeof(page_table_t));
    memset(&pdpt, 0, sizeof(page_table_t));
    memset(&pdpt_high, 0, sizeof(page_table_t));
    memset(&pdpt_hhdm, 0, sizeof(page_table_t));
    memset(&pd, 0, sizeof(page_table_t) * 32);
    memset(&pd_kernel, 0, sizeof(page_table_t));
    memset(&pt_kernel, 0, sizeof(page_table_t) * 512);

    // --- Get physical addresses ---
    struct limine_kernel_address_response* kaddr = vmm_get_kernel_address();

    uint64_t pml4_phys = ((uint64_t)&pml4 - kaddr->virtual_base) + kaddr->physical_base;
    uint64_t pdpt_phys = ((uint64_t)&pdpt - kaddr->virtual_base) + kaddr->physical_base;
    uint64_t pdpt_high_phys = ((uint64_t)&pdpt_high - kaddr->virtual_base) + kaddr->physical_base;
    uint64_t pdpt_hhdm_phys = ((uint64_t)&pdpt_hhdm - kaddr->virtual_base) + kaddr->physical_base;
    uint64_t pd_phys = ((uint64_t)&pd - kaddr->virtual_base) + kaddr->physical_base;
    uint64_t pd_kernel_phys = ((uint64_t)&pd_kernel - kaddr->virtual_base) + kaddr->physical_base;
    uint64_t pt_kernel_phys = ((uint64_t)&pt_kernel - kaddr->virtual_base) + kaddr->physical_base;


    // --- 2. IDENTITY MAP (for 0x0...) ---
    // This map is shared with the HHDM.
    pml4.entries[0] = pdpt_phys | PTE_PRESENT | PTE_WRITE | PTE_USER;

    uint64_t current_phys_addr = 0;
    for (int i = 0; i < 32; i++) {
        // Link the PDPT -> PDPT
        pdpt.entries[i] = (pd_phys + (i * PAGE_SIZE)) | PTE_PRESENT | PTE_WRITE | PTE_USER;

        // Fill the PD with 2MiB huge pages
        for (int j = 0; j < 512; j++) {
            pd[i].entries[j] = current_phys_addr | PTE_PRESENT | PTE_WRITE | PTE_HUGE_PAGE | PTE_USER;
            current_phys_addr += 0x200000; // 2MiB
        }
    }

    // --- 3. KERNEL MAP (for 0xffffffff8...) ---
    // This map is SEPARATE and uses 4KiB pages.

    uint64_t kernel_pml4_index = (kaddr->virtual_base >> 39) & 0x1FF; // 511
    uint64_t kernel_pdpt_index = (kaddr->virtual_base >> 30) & 0x1FF; // 510

    // Link PML4[511] -> pdpt_high
    pml4.entries[kernel_pml4_index] = pdpt_high_phys | PTE_PRESENT | PTE_WRITE | PTE_USER;

    // Link PDPT_HIGH[510] -> pd_kernel
    pdpt_high.entries[kernel_pdpt_index] = pd_kernel_phys | PTE_PRESENT | PTE_WRITE | PTE_USER;

    // Fill the kernel tables (1GiB)
    // This correctly maps virt 0xffffffff80000000 -> phys kaddr->physical_base
    uint64_t current_kern_phys = kaddr->physical_base;
    for (int i = 0; i < 512; i++) { // For each of the 512 PTs
        // Link the PD_KERNEL -> PT
        pd_kernel.entries[i] = (pt_kernel_phys + (i * PAGE_SIZE)) | PTE_PRESENT | PTE_WRITE | PTE_USER;

        // Fill the PT with 4KiB pages
        for (int j = 0; j < 512; j++) {
            pt_kernel[i].entries[j] = current_kern_phys | PTE_PRESENT | PTE_WRITE | PTE_USER;
            current_kern_phys += PAGE_SIZE; // 4KiB
        }
    }

    // --- 4. HIGHER-HALF IDENTITY MAP (HHDM) (for 0xffff8...) ---
    uint64_t hhdm_pml4_index = (VIRTUAL_MEMORY_OFFSET >> 39) & 0x1FF; // 256

    // Link PML4[256] -> pdpt_hhdm
    pml4.entries[hhdm_pml4_index] = pdpt_hhdm_phys | PTE_PRESENT | PTE_WRITE | PTE_USER;

    // Re-use the *same* `pd` tables from the identity map
    for (int i = 0; i < 32; i++) {
        pdpt_hhdm.entries[i] = (pd_phys + (i * PAGE_SIZE)) | PTE_PRESENT | PTE_WRITE | PTE_USER;
    }

    // --- 5. Load the new Page Map ---
    __asm__ volatile ("mov %0, %%cr3" :: "r"(pml4_phys));
}