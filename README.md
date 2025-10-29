# myOS: A 64-bit Hobby Kernel

`myOS` is a small, 64-bit hobby operating system kernel built from scratch in C and x86-64 Assembly. This project is a learning exercise in low-level systems programming, exploring everything from the boot process and memory management to preemptive multitasking and basic drivers.

*(Replace this with a screenshot of your OS in action!)*

## Current Features

The kernel currently boots in QEMU via the Limine bootloader and features:

* **64-bit Architecture:** Fully operates in x86-64 long mode.
* **Memory Management:**
    * **Paging & Virtual Memory (VMM):** A higher-half kernel with a basic page map.
    * **Physical Memory Manager (PMM):** A simple, page-based physical allocator.
    * **Kernel Heap:** A `kmalloc`/`kfree` implementation for dynamic memory.
* **Preemptive Multitasking:**
    * A preemptive, round-robin scheduler.
    * Context switching implemented in assembly, triggered by the PIT.
* **CPU Core Systems:**
    * Global Descriptor Table (GDT)
    * Interrupt Descriptor Table (IDT) for exceptions and hardware IRQs.
    * PIC for handling hardware interrupts.
* **System Call Interface:** A basic `int 0x80` syscall bridge.
* **Filesystem:**
    * Loads an `initrd.tar` (initial ramdisk) at boot.
    * A simple `tar` file parser to read files from the ramdisk.
* **Drivers:**
    * Serial Port (for debugging)
    * Framebuffer Console (for text output)
    * PS/2 Keyboard (for input)
    * Programmable Interrupt Timer (PIT) (for scheduling)
* **Interactive Kernel Shell:**
    * A modular kernel shell (`kshell`).
    * Supports commands like `help`, `clear`, `uptime`, `alloc` (PMM test), `ktest` (heap test), `syscall`, `ls`, and `cat` (initrd test).

## Building & Running

This project is built using a custom `x86_64-elf` cross-compiler and `qemu`.

### Prerequisites

* `make`
* `qemu-system-x86_64`
* `xorriso`
* A `x86_64-elf` cross-compiler toolchain (gcc, binutils).

### Quickstart

1.  **Clone the repository:**
    ```bash
    git clone [https://github.com/your-username/myos.git](https://github.com/your-username/myos.git)
    cd myos
    ```

2.  **Build and Run:**
    The included `GNUmakefile` will build the kernel, create the `initrd`, package the ISO, and launch QEMU.
    ```bash
    make run
    ```

3.  **Clean the build:**
    ```bash
    make clean
    ```

## Project Structure

The kernel source code is organized as follows:
```
src/
├── arch/         # Architecture-specific code (GDT, IDT, PMM, VMM, tasks)
├── drivers/      # Hardware drivers (serial, kbd, pit framebuffer)
├── fs/           # Filesystem code (tar parser)
├── include/      # Third-party headers (limine)
├── lib/          # Kernel libraries (heap, kshell, string)
└── main.c        # Kernel entry point (_start)
```
