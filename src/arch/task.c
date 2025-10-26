#include "task.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "serialport.h"
#include "string.h"
#include "framebuffer.h"
#include "timer.h"

// The currently running task
volatile task_t* current_task = NULL;

// The head of the "run queue" (linked list of ready tasks)
static task_t* task_queue = NULL;

static int64_t next_pid = 0;

// A simple kernel "idle task"
static void idle_task_body(void) {
    serial_write_string("Idle task started.\n");
    for (;;) {
        __asm__ volatile ("sti; hlt");
    }
}

/**
 * @brief Creates a new kernel task (shares kernel page map)
 */
task_t* create_task(void (*entry_point)(void)) {
    // Allocate the task_t structure
    task_t* task = (task_t*)kmalloc(sizeof(task_t));
    if (task == NULL) return NULL;

    memset(task, 0, sizeof(task_t));

    // Allocate a kernel stack
    task->kernel_stack = (uint8_t*)pmm_alloc_page();
    if (task->kernel_stack == NULL) {
        kfree(task);
        return NULL;
    }

    // Stack grows downwards. Set the pointer to the *top* of the stack.
    // We also map it to the higher-half.
    task->kernel_stack_ptr = (uint64_t)task->kernel_stack + KERNEL_STACK_SIZE + VIRTUAL_MEMORY_OFFSET;

    // --- Set up the initial "context" to launch the task ---
    // We set up a fake "interrupt" stack frame.
    // When we 'iretq' for the first time, it will pop these values.

    // Move stack pointer down to make space for a struct registers
    task->kernel_stack_ptr -= sizeof(struct registers);

    // 2. Get a pointer * to this new stack frame *
    struct registers* frame = (struct registers*)task->kernel_stack_ptr;

    // 3. Zero out the entire frame to avoid garbage values
    memset(frame, 0, sizeof(struct registers));

    // 4. Now, write the values for 'iretq' *directly to the stack*
    frame->rip = (uint64_t)entry_point; // Set instruction pointer
    frame->rflags = 0x202; // Enable interrupts (IF flag)
    frame->cs = 0x08; // Kernel Code Segment
    frame->ss = 0x10; // Kernel Data Segment
    frame->rsp = task->kernel_stack_ptr + sizeof(struct registers);

    // Set other fields
    task->pid = next_pid++;
    task->state = TASK_STATE_READY;
    task->pml4 = vmm_get_kernel_pml4(); // All kernel tasks share Paging

    // Add to the task queue
    if (task_queue == NULL) {
        task_queue = task;
        task->next = task; // Points to itself (circular)
    }
    else {
        task->next = task_queue->next;
        task_queue->next = task;
    }

    return task;
}

/**
 * @brief Initializes the tasking system
 */
void task_init(void) {
    serial_write_string("Initializing multitasking...\n");

    // Create the idle task
    task_t* idle_task = create_task(idle_task_body);
    if (idle_task == NULL) {
        serial_write_string("PANIC: Failed to create idle task!\n");
        // We can't continue without a task
        for (;;) __asm__ volatile ("cli; hlt");
    }

    // Set it as the "running" task
    current_task = idle_task;
    current_task->state = TASK_STATE_RUNNING;

    serial_write_string("Multitasking initialized.\n");
}

/**
 * @brief The main scheduler function.
 * Called by the timer interrupt assembly stub.
 *
 * @param old_regs A pointer to the saved registers on the old task's stack.
 * @return The stack pointer (RSP) of the *new* task to switch to.
 */
void* __attribute__((used)) schedule_and_switch(struct registers* old_regs) {
    // Increment the global timer tick. This is the new home
    // for this logic.
    timer_tick();
    if (current_task == NULL) {
        // This should *never* happen if task_init() ran.
        // But if it does, just return the same stack.
        return (void*)old_regs;
    }

    // 1. Save the old task's context
    // The assembly stub saved the registers *on the stack*.
    // We just need to save the *pointer* to that stack frame.
    current_task->regs = *old_regs;
    current_task->kernel_stack_ptr = (uint64_t)old_regs;

    // 2. Find the next task to run (simple round-robin)
    task_t* next = current_task->next;
    while (next->state != TASK_STATE_READY && next != current_task) {
        // Note: This is a potential deadlock if all tasks are SLEEPING.
        // For now, our idle_task is always READY.
        next = next->next;
    }

    // 3. Update task states
    if (current_task->state == TASK_STATE_RUNNING) {
        current_task->state = TASK_STATE_READY;
    }

    next->state = TASK_STATE_RUNNING;
    current_task = next;

    // 4. Return the new task's stack pointer
    // The assembly stub will load this into RSP.
    return (void*)current_task->kernel_stack_ptr;
}