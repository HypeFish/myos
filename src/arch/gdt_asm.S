#
# This file is now in AT&T syntax (source, destination)
#
.section .text

# Make these functions visible to the C code
.global load_gdt
.global reload_segments

# void load_gdt(struct gdt_descriptor *desc);
# The first argument is passed in the %rdi register.
load_gdt:
    # Load the GDT descriptor from the address in %rdi
    lgdt (%rdi)
    ret

# void reload_segments(void);
reload_segments:
    # Reload the Code Segment register (CS)
    # $0x08 is the selector for our 2nd entry (64-bit Kernel Code)
    pushq $0x08
    leaq .reload_cs(%rip), %rax
    pushq %rax
    retfq
.reload_cs:
    # Reload the Data Segment registers (DS, ES, FS, GS, SS)
    # $0x10 is the selector for our 3rd entry (64-bit Kernel Data)
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    ret