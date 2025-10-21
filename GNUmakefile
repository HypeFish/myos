# This is the name that our final executable will have.
override OUTPUT := myos

# Set the toolchain prefix
TOOLCHAIN_PREFIX ?= x86_64-elf-
CC := $(TOOLCHAIN_PREFIX)gcc
LD := $(TOOLCHAIN_PREFIX)ld

# --- Compiler and Linker Flags ---
override CFLAGS += \
    -Wall \
    -Wextra \
    -std=gnu11 \
    -ffreestanding \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-lto \
    -fno-PIC \
    -ffunction-sections \
    -fdata-sections \
    -m64 \
    -march=x86-64 -msse3 \
    -mabi=sysv \
    -mno-red-zone \
    -mcmodel=kernel \
    -g

override CPPFLAGS += \
    -I src \
    -DLIMINE_API_REVISION=3 \
    -MMD \
    -MP

override LDFLAGS += \
    -m elf_x86_64 \
    -nostdlib \
    -static \
    -z max-page-size=0x1000 \
    --gc-sections \
    -T linker.lds

# --- Find Source Files ---
CFILES := $(shell find -L src -type f -name '*.c' 2>/dev/null | LC_ALL=C sort)
ASFILES := $(shell find -L src -type f -name '*.S' 2>/dev/null | LC_ALL=C sort)

COBJ := $(patsubst src/%.c, obj/%.o, $(CFILES))
ASOBJ := $(patsubst src/%.S, obj/%.o, $(ASFILES))
OBJ := $(COBJ) $(ASOBJ)
HEADER_DEPS := $(patsubst src/%.c, obj/%.d, $(CFILES))

# --- Build Rules ---

# Default target
.PHONY: all
all: bin/$(OUTPUT)

-include $(HEADER_DEPS)

bin/$(OUTPUT): linker.lds $(OBJ)
	@mkdir -p "$(dir $@)"
	$(LD) $(LDFLAGS) $(OBJ) -o $@

obj/%.o: src/%.c GNUmakefile
	@mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

obj/%.o: src/%.S GNUmakefile
	@mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Rule to clean the project directory
.PHONY: clean
clean:
	@rm -rf bin obj *.iso iso_root limine

# Phony target for limine download/build if it doesn't exist
.PHONY: limine
limine:
	@test -d limine || git clone https://github.com/limine-bootloader/limine.git --branch=v3.0-branch-binary --depth=1 limine
	@$(MAKE) -C limine

# Rule to build the ISO image
.PHONY: image.iso
image.iso: all limine.cfg limine
	@echo "Building ISO image..."
	@rm -rf iso_root
	@mkdir -p iso_root/boot/
	@cp -v bin/$(OUTPUT) iso_root/boot/myos
	@cp -v limine.cfg limine/limine.sys limine/limine-cd.bin iso_root/boot/
	@mkdir -p iso_root/EFI/BOOT
	@cp -v limine/BOOTX64.EFI iso_root/EFI/BOOT/
	@cp -v limine/BOOTIA32.EFI iso_root/EFI/BOOT/
	@cp -v limine.cfg iso_root/EFI/BOOT/
	@xorriso -as mkisofs -b boot/limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot EFI/BOOT/BOOTX64.EFI \
		-efi-boot-part --efi-boot-image \
		iso_root -o image.iso
	@./limine/limine-deploy image.iso
	@rm -rf iso_root

# Rule to run QEMU
.PHONY: run
run: image.iso
	@qemu-system-x86_64 -cdrom image.iso \
	-no-reboot -no-shutdown \
	-serial stdio

