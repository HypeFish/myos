#ifndef __PAGING_H__
#define __PAGING_H__

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 0x1000 // 4KiB

// We will map all physical memory to this virtual offset
#define VIRTUAL_MEMORY_OFFSET 0xffff800000000000ull

// The kernel will be mapped to this virtual base address
// (This should match your linker script)
#define KERNEL_VIRTUAL_BASE 0xffffffff80000000ull

// --- Page Table Entry (PTE) Flags ---
// (Based on the OSDev wiki page)
#define PTE_PRESENT       (1ull << 0)  // Page is present in memory
#define PTE_WRITE         (1ull << 1)  // Page is writeable
#define PTE_USER          (1ull << 2)  // Page is accessible by user-mode
#define PTE_WRITE_THROUGH (1ull << 3)  // Write-through caching
#define PTE_CACHE_DISABLE (1ull << 4)  // Disable caching
#define PTE_ACCESSED      (1ull << 5)  // Page was accessed
#define PTE_DIRTY         (1ull << 6)  // Page was written to (for page directory)
#define PTE_HUGE_PAGE     (1ull << 7)  // Page is 2MiB (for page directory)
#define PTE_GLOBAL        (1ull << 8)  // Page is global (not flushed from TLB)
#define PTE_NO_EXECUTE    (1ull << 63) // Page cannot be executed (if NXE bit set in EFER)

// Mask to get the physical address from an entry
#define PTE_ADDR_MASK     0x000FFFFFFFFFF000ull

// --- Paging Structures ---
// Each table has 512 entries, and each entry is 8 bytes (64 bits)
#define ENTRIES_PER_TABLE 512

// Page Map Level 4 (PML4) Entry
typedef uint64_t pml4_entry_t;

// Page Directory Pointer Table (PDPT) Entry
typedef uint64_t pdpt_entry_t;

// Page Directory (PD) Entry
typedef uint64_t pd_entry_t;

// Page Table (PT) Entry
typedef uint64_t pt_entry_t;


// A full 4KiB page table
__attribute__((aligned(PAGE_SIZE)))
typedef struct {
    uint64_t entries[ENTRIES_PER_TABLE];
} page_table_t;


// --- Address Calculation ---
// Helper macros to get indices from a virtual address
#define PML4_INDEX(virt) (((virt) >> 39) & 0x1FF)
#define PDPT_INDEX(virt) (((virt) >> 30) & 0x1FF)
#define PD_INDEX(virt)   (((virt) >> 21) & 0x1FF)
#define PT_INDEX(virt)   (((virt) >> 12) & 0x1FF)
#define OFFSET_INDEX(virt) ((virt) & 0xFFF)

#endif // __PAGING_H__