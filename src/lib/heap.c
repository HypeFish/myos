#include "heap.h"
#include "pmm.h"
#include "serialport.h" // For serial_write_string
#include <stddef.h>
#include <stdbool.h>

// A header for each free memory block
typedef struct heap_block {
    size_t size;
    struct heap_block* next;
} heap_block_t;

// The start of our free list
static heap_block_t* free_list_head = NULL;

/**
 * @brief Requests more memory from the PMM to add to the heap.
 * @return true on success, false if the PMM is out of memory.
 */
static bool request_more_memory() {
    // Request one page for now
    void* p = pmm_alloc_page();
    if (p == NULL) {
        serial_write_string("ERROR: kmalloc failed to get new page from PMM\n");
        return false;
    }
    
    // Treat this new page as one giant free block
    heap_block_t* new_block = (heap_block_t*)p;
    new_block->size = 4096 - sizeof(heap_block_t); // Size is page size minus our header
    new_block->next = NULL;

    // kfree will add it to the free list for us
    kfree((void*)(new_block + 1));
    return true;
}

void heap_init(void) {
    free_list_head = NULL;
    serial_write_string("Kernel heap initialized.\n");
}

void* kmalloc(size_t size) {
    // We must align to the size of our header for sanity
    if (size < sizeof(heap_block_t)) {
        size = sizeof(heap_block_t);
    }
    
    // For simplicity, let's also align to 16 bytes for 64-bit
    size = (size + 15) & ~15;

    // Outer loop to handle requesting more memory
    while (true) {
        heap_block_t* curr = free_list_head;
        heap_block_t* prev = NULL;

        // --- 1. Search the free list for a block ---
        while (curr != NULL) {
            if (curr->size >= size) {
                // Found a block!
                
                // Can we split it? We split if the remaining space
                // is large enough to hold a new block header.
                if (curr->size > size + sizeof(heap_block_t)) {
                    // Yes, split the block.
                    // 1. Create a new header for the *remaining* part.
                    heap_block_t* new_free_block = (heap_block_t*)((char*)curr + sizeof(heap_block_t) + size);
                    new_free_block->size = curr->size - size - sizeof(heap_block_t);
                    new_free_block->next = curr->next;
                    
                    // 2. Link the list to this new free block.
                    if (prev == NULL) {
                        free_list_head = new_free_block;
                    } else {
                        prev->next = new_free_block;
                    }
                    
                    // 3. Resize the block we are returning.
                    curr->size = size;

                } else {
                    // No, we can't split. Use the whole block.
                    // Just unlink it from the list.
                    if (prev == NULL) {
                        free_list_head = curr->next;
                    } else {
                        prev->next = curr->next;
                    }
                }
                
                curr->next = NULL; // Not on the free list anymore
                
                // Return a pointer to the data area (just *after* the header)
                return (void*)(curr + 1);
            }
            
            // Not big enough, check next block
            prev = curr;
            curr = curr->next;
        }

        // --- 2. No block found. Request more memory from PMM. ---
        if (request_more_memory() == false) {
            // PMM is out of memory, we can't satisfy the request.
            serial_write_string("PANIC: Kernel heap is out of memory!\n");
            return NULL;
        }
        
        // If we got here, request_more_memory() succeeded.
        // The `while(true)` loop will now run again to find the new block.
    }
}

void kfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    // Get the header, which is right before the data pointer
    heap_block_t* block = (heap_block_t*)ptr - 1;

    // Add this block to the front of the free list
    block->next = free_list_head;
    free_list_head = block;
}