# Tell the assembler to use Intel syntax (destination, source)
# and not to require '%' before register names.
.intel_syntax noprefix

.section .text

# Make these functions visible to the C code
.global load_gdt
.global reload_segments

# void load_gdt(struct gdt_descriptor *desc);
# The first argument is passed in the RDI register in the System V ABI.
load_gdt:
    # Load the GDT descriptor from the address in RDI
    lgdt [rdi]
    ret

# void reload_segments(void);
reload_segments:
    # Reload the Code Segment register (CS)
    # 0x08 is the selector for our 2nd entry (64-bit Kernel Code)
    push 0x08
    lea rax, [rip + .reload_cs]
    push rax
    retfq
.reload_cs:
    
    # Reload the Data Segment registers (DS, ES, FS, GS, SS)
    # 0x10 is the selector for our 3rd entry (64-bit Kernel Data)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret
