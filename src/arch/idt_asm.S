.section .text

# Make these functions visible to C code
.global load_idt
.global isr_stub_0   # Divide by zero
.global isr_stub_8   # Double Fault

.global isr_stub_default # For all other interrupts

# Declare the C function we will callS
.extern exception_handler
.extern irq_handler # C handler for IRQs
.extern syscall_handler # C handler for syscalls
.extern schedule_and_switch # C function to handle scheduling

# IRQ stubs
.global irq_stub_32 # Timer
.global irq_stub_33 # Keyboard
.global irq_stub_34 # Cascade
.global irq_stub_35 # COM2
.global irq_stub_36 # COM1
.global irq_stub_37 # LPT2
.global irq_stub_38 # Floppy
.global irq_stub_39 # LPT1
.global irq_stub_40 # RTC
.global irq_stub_41 # IRQ 9
.global irq_stub_42 # IRQ 10
.global irq_stub_43 # IRQ 11
.global irq_stub_44 # IRQ 12
.global irq_stub_45 # IRQ 13
.global irq_stub_46 # IRQ 14
.global irq_stub_47 # IRQ 15
.global isr_stub_128 # System Call

# void load_idt(struct idt_descriptor *desc);
# The first argument is passed in the RDI register.
load_idt:
    # Load the IDT descriptor from the address in %rdi
    lidt (%rdi)
    ret

# Exception and Interrupt Stubs
.macro ISR_STUB_NO_ERR vector
    # This stub is for interrupts that DO NOT push an error code (
    .global isr_stub_\vector
    isr_stub_\vector:
        cli             # Disable interrupts
        push $0         # Push a dummy error code
        push $\vector   # Push the interrupt number (immediate value)
        jmp common_isr_stub
.endm

.macro ISR_STUB_ERR vector
    # This stub is for interrupts that DO push an error code 
    .global isr_stub_\vector
    isr_stub_\vector:
        cli             # Disable interrupts
        # Error code is already on the stack
        push $\vector   # Push the interrupt number (immediate value)
        jmp common_isr_stub
.endm

# Create stubs for the first 32 exceptions
ISR_STUB_NO_ERR 0   # 0: Divide by Zero
ISR_STUB_NO_ERR 1   # 1: Debug
ISR_STUB_NO_ERR 2   # 2: NMI
ISR_STUB_NO_ERR 3   # 3: Breakpoint
ISR_STUB_NO_ERR 4   # 4: Overflow
ISR_STUB_NO_ERR 5   # 5: Bound Range Exceeded
ISR_STUB_NO_ERR 6   # 6: Invalid Opcode
ISR_STUB_NO_ERR 7   # 7: Device Not Available
ISR_STUB_ERR    8   # 8: Double Fault
ISR_STUB_NO_ERR 9   # 9: Coprocessor Segment Overrun (legacy)
ISR_STUB_ERR    10  # 10: Invalid TSS
ISR_STUB_ERR    11  # 11: Segment Not Present
ISR_STUB_ERR    12  # 12: Stack-Segment Fault
ISR_STUB_ERR    13  # 13: General Protection Fault
ISR_STUB_ERR    14  # 14: Page Fault
ISR_STUB_NO_ERR 15  # 15: (Reserved)
ISR_STUB_NO_ERR 16  # 16: x87 FPU Error
ISR_STUB_ERR    17  # 17: Alignment Check
ISR_STUB_NO_ERR 18  # 18: Machine Check
ISR_STUB_NO_ERR 19  # 19: SIMD FPU Exception
ISR_STUB_NO_ERR 20  # 20: Virtualization Exception
ISR_STUB_ERR    21  # 21: Control Protection Exception
# ... 22-27 are reserved
ISR_STUB_NO_ERR 28  # 28: Hypervisor Injection Exception
ISR_STUB_ERR    29  # 29: VMM Communication Exception
ISR_STUB_ERR    30  # 30: Security Exception
ISR_STUB_NO_ERR 31  # 31: (Reserved)

# Syscall Stub (vector 128 / 0x80)
isr_stub_128:
    cli         # Disable interrupts
    push $0     # Push a dummy error code (int 0x80 doesn't)
    push $128   # Push the interrupt number
    jmp common_syscall_stub

# Default ISR Stub for other interrupts (vectors 32-255)
isr_stub_default:
    cli
    push $0     # No error code
    push $255   # A special vector number for "unknown"
    jmp common_isr_stub

common_isr_stub:
    # Save all general-purpose registers
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    # Call the C handler. The stack pointer (%rsp) now points to
    # the saved registers.
    # We pass the stack pointer itself as the first argument (in %rdi).
    mov %rsp, %rdi
    call exception_handler

    # Restore all registers (in reverse order)
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax

    # Pop the interrupt number and error code
    add $16, %rsp

    # Return from interrupt
    iretq

# IRQ Stubs (vectors 32-47)
.macro IRQ_STUB vector, jmp_target
    .global irq_stub_\vector
    irq_stub_\vector:
        cli             # Disable interrupts
        push $0         # Push a dummy error code (IRQs don't have one)
        push $\vector   # Push the interrupt number (32-47)
        jmp \jmp_target
.endm

# Create stubs for all 16 IRQs (vectors 32-47)
# IRQ 0 (Timer) jumps to the new scheduler stub
IRQ_STUB 32, common_irq_sched_stub

# All other IRQs jump to the normal stub
IRQ_STUB 33, common_irq_stub
IRQ_STUB 34, common_irq_stub
IRQ_STUB 35, common_irq_stub
IRQ_STUB 36, common_irq_stub
IRQ_STUB 37, common_irq_stub
IRQ_STUB 38, common_irq_stub
IRQ_STUB 39, common_irq_stub
IRQ_STUB 40, common_irq_stub
IRQ_STUB 41, common_irq_stub
IRQ_STUB 42, common_irq_stub
IRQ_STUB 43, common_irq_stub
IRQ_STUB 44, common_irq_stub
IRQ_STUB 45, common_irq_stub
IRQ_STUB 46, common_irq_stub
IRQ_STUB 47, common_irq_stub

common_irq_stub:
    # Save all general-purpose registers
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    # Call the C handler.
    mov %rsp, %rdi
    call irq_handler # Call the new C handler for IRQs

    # Restore all registers
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax

    # Pop the interrupt number and error code
    add $16, %rsp

    # Return from interrupt
    iretq


# Syscall Stub
.global common_syscall_stub
common_syscall_stub:
    # Save all general-purpose registers
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    # Pass a pointer to the saved registers (%rsp)
    # as the first argument (in %rdi) to the C function
    mov %rsp, %rdi

    # Call the C-level handler
    call syscall_handler

    # Restore all registers
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rbx
    
    # The C handler put its return value in 'rax' on the stack.
    # We pop it into %rax here to preserve it.
    pop %rax

    # Pop int_no and err_code
    add $16, %rsp

    # Return from interrupt, re-enabling interrupts
    sti
    iretq


# Common IRQ Scheduler Stub (for IRQ 0 - Timer)
.global common_irq_sched_stub
common_irq_sched_stub:
    # 1. Save all registers (this is 'struct registers')
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
    
    # 2. Pass pointer to the 'struct registers' (%rsp) to C
    mov %rsp, %rdi
    
    # 3. Call the C scheduler.
    # It will save the RSP (rdi) into current_task->regs
    # and return the *new* task's RSP in RAX
    call schedule_and_switch
    
    # 4. %rax now contains the *new* stack pointer.
    # Switch stacks.
    mov %rax, %rsp
    
    # 5. Restore all registers for the *new* task
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax
    
    # 6. Pop int_no and err_code (which were part of the saved frame)
    add $16, %rsp
    
    # 7. Send EOI (End of Interrupt) to the PIC
    # This must be done *after* switching tasks.
    mov $0x20, %al
    outb %al, $0x20 # EOI to master PIC

    # 8. Return from interrupt, restoring RIP, CS, RFLAGS, RSP, SS
    # of the *new* task.
    iretq