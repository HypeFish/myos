#include "idt.h"          // For struct registers
#include "serialport.h"   // For debugging
#include "framebuffer.h"  // For fb_print
#include "string.h"   // For strlen (or just a simple one)

// A simple strlen just for this test
static size_t simple_strlen(const char* s) {
    size_t i = 0;
    while (s[i]) i++;
    return i;
}

/**
 * This is our C-level syscall handler.
 * It's called from 'common_syscall_stub' in idt_asm.S
 * The 'struct registers' contains the state of the CPU
 * at the time of the 'int 0x80' instruction.
 */
void __attribute__((used)) syscall_handler(struct registers* regs) {

    // Syscall number is passed in the RAX register
    uint64_t syscall_num = regs->rax;
    uint64_t ret_val = 0;

    switch (syscall_num) {

        /**
         * Syscall 0: SYS_WRITE
         * arg0 (RDI): file descriptor (e.g., 1 for stdout)
         * arg1 (RSI): pointer to string
         * arg2 (RDX): string length
         */
    case 0: {
        uint64_t fd = regs->rdi;
        char* str = (char*)regs->rsi;
        size_t len = (size_t)regs->rdx; // Not used for now, but good to have

        // We only handle "stdout" (the framebuffer) for now
        if (fd == 1) {
            // We'll just use fb_print, which expects a null-terminated string
            // A real implementation would use 'len'
            fb_print(str);

            // Also log to serial for debugging
            serial_write_string("[SYSCALL] write(1, \"");
            serial_write_string(str);
            serial_write_string("\")\n");

            // Return value (e.g., number of bytes written)
            ret_val = simple_strlen(str);
        }
        break;
    }

          // Add more cases here for future syscalls...
          // case 1: // SYS_READ
          // case 2: // SYS_MALLOC
          //    break;

    default:
        serial_write_string("ERROR: Unknown syscall number!\n");
        ret_val = (uint64_t)-1; // -1 (error)
        break;
    }

    // The return value is passed back to the caller in RAX
    regs->rax = ret_val;
}