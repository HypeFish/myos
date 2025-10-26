#ifndef __TASK_H__
#define __TASK_H__

#include <stdint.h>
#include <stddef.h>
#include "idt.h" // For struct registers
#include "paging.h" // For page_table_t

#define KERNEL_STACK_SIZE 4096 // 4KiB kernel stack per process

typedef enum {
    TASK_STATE_READY,     // Ready to be scheduled
    TASK_STATE_RUNNING,   // Currently running
    TASK_STATE_SLEEPING,  // Waiting for an event
    TASK_STATE_DEAD       // Marked for deletion
} task_state_t;

typedef struct task {
    // --- Context switching info ---
    // This MUST be the first element.
    // It holds the saved register state when the task is not running.
    // We will save/restore this in assembly.
    struct registers regs;

    // --- Kernel stack ---
    // We need a separate kernel stack for each task
    // so they don't overwrite each other during interrupts.
    uint8_t* kernel_stack;
    uint64_t kernel_stack_ptr; // The top of the kernel stack (RSP)

    // --- Scheduling info ---
    int64_t pid;                // Process ID
    task_state_t state;         // Current task state

    // --- Paging info ---
    page_table_t* pml4;         // Pointer to this task's page map

    // --- Linked list for scheduler ---
    struct task* next;

} task_t;

// --- Public Functions ---

/**
 * @brief Initializes the tasking system and creates the first kernel task.
 */
void task_init(void);


/**
 * @brief Creates a new kernel task (shares kernel page map)
 * @param entry_point The function (RIP) where the task will begin execution.
 * @return A pointer to the new task, or NULL on failure.
 */
task_t* create_task(void (*entry_point)(void)); // <-- ADD THIS PROTOTYPE

#endif // __TASK_H__