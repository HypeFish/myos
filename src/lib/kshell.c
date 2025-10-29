#include "kshell.h"
#include "framebuffer.h" // For fb_print, fb_putchar, etc.
#include "string.h"  // For strcmp
#include "pmm.h"         // For alloc command
#include "heap.h"        // For ktest command
#include "timer.h"       // For uptime command
#include "tar.h"        

// --- Shell Buffer  ---
static char line_buffer[256];
static int buffer_index = 0;
#define MAX_BUFFER 255

// --- Print Helpers ---
static void fb_print_uint(uint64_t n) {
    if (n == 0) {
        fb_putchar('0');
        return;
    }
    char buffer[20];
    int i = 19;
    buffer[i] = '\0';
    while (n > 0) {
        i--;
        buffer[i] = (n % 10) + '0';
        n /= 10;
    }
    fb_print(&buffer[i]);
}

static void fb_print_hex(uint64_t n) {
    char hex_chars[] = "0123456789ABCDEF";
    fb_print("0x");
    for (int i = 60; i >= 0; i -= 4) {
        char c = hex_chars[(n >> i) & 0xF];
        fb_putchar(c);
    }
}

// --- Command Execution ---
static void shell_execute(const char* command) {
    if (strcmp(command, "help") == 0) {
        fb_print("Welcome to myOS!\n");
        fb_print("Available commands: help, clear, panic, alloc, ktest, uptime, syscall, cat, ls\n");
    }
    else if (strcmp(command, "clear") == 0) {
        fb_clear();
    }
    else if (strcmp(command, "panic") == 0) {
        fb_print("Triggering test panic (Divide by Zero).\n");
        volatile int zero = 0;
        volatile int panic = 1 / zero;
        (void)panic;
    }
    else if (strcmp(command, "alloc") == 0) {
        fb_print("Allocating one page...\n");
        void* p = pmm_alloc_page();
        if (p != NULL) {
            fb_print("  Successfully allocated 4KiB at: ");
            fb_print_hex((uint64_t)p);
            fb_print("\n  Freeing it back...\n");
            pmm_free_page(p);
        }
        else {
            fb_print("  Allocation failed! Out of memory.\n");
        }
    }
    else if (strcmp(command, "ktest") == 0) {
        fb_print("Testing kernel heap (kmalloc)...\n");

        fb_print("  Allocating 30 bytes (a1)...\n");
        void* a1 = kmalloc(30);
        fb_print("  Allocated at: "); fb_print_hex((uint64_t)a1); fb_print("\n");

        fb_print("  Allocating 500 bytes (a2)...\n");
        void* a2 = kmalloc(500);
        fb_print("  Allocated at: "); fb_print_hex((uint64_t)a2); fb_print("\n");

        fb_print("  Freeing a1...\n");
        kfree(a1);

        fb_print("  Allocating 30 bytes (a3)...\n");
        void* a3 = kmalloc(30);
        fb_print("  Allocated at: "); fb_print_hex((uint64_t)a3); fb_print("\n");

        fb_print("  Freeing a2 and a3...\n");
        kfree(a2);
        kfree(a3);

        fb_print("Heap test complete.\n");
    }
    else if (strcmp(command, "uptime") == 0) {
        uint64_t current_ticks = get_ticks();
        uint64_t seconds = current_ticks / 100;
        uint64_t centiseconds = current_ticks % 100;
        fb_print("Uptime: ");
        fb_print_uint(seconds);
        fb_print(".");
        if (centiseconds < 10) {
            fb_print("0");
        }
        fb_print_uint(centiseconds);
        fb_print(" seconds\n");
    }
    else if (strcmp(command, "syscall") == 0) {
        fb_print("\nIssuing test SYS_WRITE (int 0x80)...\n");

        char* test_str = "  ...Hello from syscall 0! (stdout)\n";
        uint64_t len = 34;
        uint64_t ret;

        __asm__ volatile (
            "int $0x80"
            : "=a" (ret)
            : "a" (0), "D" (1), "S" (test_str), "d" (len)
            : "memory", "rcx", "r11"
            );

        fb_print("  Syscall returned (bytes written): ");
        fb_print_hex(ret);
        fb_print("\n");
    }
    else if (strcmp(command, "ls") == 0) {
        tar_list_files();
    }
    else if (strcmp(command, "cat") == 0) {
        fb_print("Reading from initrd/hello.txt...\n");
        
        char* content = (char*)tar_lookup("hello.txt");
        
        if (content != NULL) {
            fb_print("Content of hello.txt:\n");
            fb_print(content);
            fb_print("\n");
        } else {
            fb_print("ERROR: Could not find hello.txt!\n");
        }
    }
    else if (strcmp(command, "") == 0) {
        // Do nothing
    }
    else {
        fb_print("Unknown command: ");
        fb_print(command);
        fb_print("\n");
    }
}

// --- Public Functions ---

void kshell_init(void) {
    // Print the initial prompt
    fb_print("> ");
}

void kshell_process_char(char c) {
    // This is the logic from idt.c's keyboard handler
    if (c == '\n') {
        fb_putchar('\n');
        line_buffer[buffer_index] = '\0'; // Null-terminate
        shell_execute(line_buffer);      // Process command
        buffer_index = 0;                  // Reset buffer
        fb_print("> ");                    // Print new prompt
    }
    else if (c == '\b') {
        if (buffer_index > 0) {
            buffer_index--;
            fb_putchar('\b'); // fb_putchar handles the erase
        }
    }
    else if (c != 0) {
        if (buffer_index < MAX_BUFFER) {
            line_buffer[buffer_index] = c;
            buffer_index++;
            fb_putchar(c); // Echo character to screen
        }
    }
}

void kshell_print_hex(uint64_t n) { 
    char hex_chars[] = "0123456789ABCDEF";
    fb_print("0x");
    for (int i = 60; i >= 0; i -= 4) {
        char c = hex_chars[(n >> i) & 0xF];
        fb_putchar(c);
    }
}

void kshell_print_uint(uint64_t n) { 
    if (n == 0) {
        fb_putchar('0');
        return;
    }
    char buffer[20];
    int i = 19;
    buffer[i] = '\0';
    while (n > 0) {
        i--;
        buffer[i] = (n % 10) + '0';
        n /= 10;
    }
    fb_print(&buffer[i]);
}