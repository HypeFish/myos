# Tell the assembler to use Intel syntax (destination, source)
# and not to require '%' before register names.
.intel_syntax noprefix

.section .text

# --- Exception (ISR) Section ---
# Make these functions visible to C code
.global load_idt
.global isr_stub_0   # Divide by zero
.global isr_stub_8   # Double Fault
# ... (you would add all 32 here)
.global isr_stub_default # For all other interrupts

# Declare the C function we will call
.extern exception_handler

# --- NEW: IRQ (Hardware Interrupt) Section ---
.global irq_stub_32 # Timer
.global irq_stub_33 # Keyboard
.global irq_stub_34
.global irq_stub_35
.global irq_stub_36
.global irq_stub_37
.global irq_stub_38
.global irq_stub_39
.global irq_stub_40
.global irq_stub_41
.global irq_stub_42
.global irq_stub_43
.global irq_stub_44
.global irq_stub_45
.global irq_stub_46
.global irq_stub_47

.extern irq_handler # New C handler for hardware IRQs
# --- END NEW ---

# void load_idt(struct idt_descriptor *desc);
# The first argument is passed in the RDI register.
load_idt:
    # Load the IDT descriptor from the address in RDI
    lidt [rdi]
    ret

# --- Exception Stubs (ISR) ---
.macro ISR_STUB_NO_ERR vector
    # This stub is for interrupts that DO NOT push an error code (like \vector)
    .global isr_stub_\vector
    isr_stub_\vector:
        cli             # Disable interrupts
        push 0          # Push a dummy error code
        push \vector    # Push the interrupt number (immediate value)
        jmp common_isr_stub
.endm

.macro ISR_STUB_ERR vector
    # This stub is for interrupts that DO push an error code (like \vector)
    .global isr_stub_\vector
    isr_stub_\vector:
        cli             # Disable interrupts
        # Error code is already on the stack
        push \vector    # Push the interrupt number (immediate value)
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

# A default handler for vectors 32-255 (e.g., hardware interrupts)
# You can make this more specific later.
isr_stub_default:
    cli
    push 0  # No error code
    push 255 # A special vector number for "unknown"
    jmp common_isr_stub

common_isr_stub:
    # Save all general-purpose registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    # Call the C handler. The stack pointer (RSP) now points to
    # the saved registers, which is the 'struct registers*' argument.
    # We pass the stack pointer itself as the argument.
    mov rdi, rsp
    call exception_handler

    # Restore all registers (in reverse order)
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    # Pop the interrupt number and error code
    add rsp, 16

    # Return from interrupt
    iretq

# --- NEW: Hardware Interrupt Stubs (IRQ) ---
# We use a separate macro and common stub for clarity
.macro IRQ_STUB vector
    .global irq_stub_\vector
    irq_stub_\vector:
        cli             # Disable interrupts
        push 0          # Push a dummy error code (IRQs don't have one)
        push \vector    # Push the interrupt number (32-47)
        jmp common_irq_stub
.endm

# Create stubs for all 16 IRQs (vectors 32-47)
IRQ_STUB 32
IRQ_STUB 33
IRQ_STUB 34
IRQ_STUB 35
IRQ_STUB 36
IRQ_STUB 37
IRQ_STUB 38
IRQ_STUB 39
IRQ_STUB 40
IRQ_STUB 41
IRQ_STUB 42
IRQ_STUB 43
IRQ_STUB 44
IRQ_STUB 45
IRQ_STUB 46
IRQ_STUB 47

common_irq_stub:
    # Save all general-purpose registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    # Call the C handler.
    mov rdi, rsp
    call irq_handler # Call the new C handler for IRQs

    # Restore all registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    # Pop the interrupt number and error code
    add rsp, 16

    # Return from interrupt
    iretq
# --- END NEW ---
