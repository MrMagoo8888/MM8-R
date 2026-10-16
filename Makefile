COLOUR_GREEN=\033[0;32m
COLOUR_RED=\033[0;31m
COLOUR_BLUE=\033[0;34m
COLOUR_END=\033[0m

TOOLCHAIN_GOALS := toolchain_all toolchain_32 toolchain_64 clean-toolchain clean-toolchain-all

ifneq ($(filter $(TOOLCHAIN_GOALS),$(MAKECMDGOALS)),)
include build_scripts/config.mk
include build_scripts/toolchain.mk
endif



ARCH        ?= x86_64
SRC_DIR     := src
BUILD_DIR   := build/$(ARCH)
DIST_DIR    := dist/$(ARCH)
TARGET_DIR  := targets/$(ARCH)

TOOLCHAIN_BIN := toolchain/x86_64-elf/bin
NASM        := nasm
LD          := $(TOOLCHAIN_BIN)/x86_64-elf-ld
CC          := $(TOOLCHAIN_BIN)/x86_64-elf-gcc
GRUB_MKRESCUE ?= $(shell command -v grub-mkrescue 2>/dev/null || command -v grub2-mkrescue 2>/dev/null)

CFLAGS      := -m64 -ffreestanding -fno-builtin -O2 -Wall -Wextra \
				 -DCONFIG_USE_DEFAULT_CFG=1 -DCONFIG_HAVE_OWN_ERRNO=1 \
				 -I$(SRC_DIR)/kernel/include \
				 -I$(SRC_DIR)/arch/x86_64/include \
				 -I$(SRC_DIR)/arch/x86_64/interrupts \
				 -I$(SRC_DIR)/kernel/fs/ext4/include

ASM_FLAGS   := -f elf64
LD_FLAGS    := -n -T $(TARGET_DIR)/linker.ld

rwildcard    = $(foreach d,$(wildcard $(1)/*),$(call rwildcard,$(d),$(2)) $(filter $(subst *,%,$(2)),$(d)))


# find files
ASM_SRCS_ASM    := $(strip $(call rwildcard,$(SRC_DIR)/arch,*.asm))
ASM_SRCS_INC    := $(strip $(call rwildcard,$(SRC_DIR)/arch,*.inc))
ASM_SRCS		:= $(strip $(ASM_SRCS_ASM) $(ASM_SRCS_INC))

ASM_OBJS     := $(patsubst src/%.asm,build/$(ARCH)/%.o,$(filter %.asm,$(ASM_SRCS))) \
                $(patsubst src/%.inc,build/$(ARCH)/%.o,$(filter %.inc,$(ASM_SRCS)))

C_SRCS      := $(strip $(call rwildcard,$(SRC_DIR)/,*.c))
C_OBJS      := $(patsubst src/%.c,build/$(ARCH)/%.o,$(C_SRCS))

KERNEL_BIN  := $(DIST_DIR)/kernel.bin
KERNEL_ISO  := $(DIST_DIR)/kernel.iso
BOOT_BIN    := $(TARGET_DIR)/iso/boot/kernel.bin

ISO_DIR		= $(TARGET_DIR)/iso
ROOT_DIR	= $(TARGET_DIR)/root

# fail if no asm found
ifeq ($(ASM_SRCS),)
$(error No .asm source files found in '$(SRC_DIR)'. Please check directory path and file extensions)
endif


# Targets und Rules
.DEFAULT_GOAL := all

.PHONY: all clean build-$(ARCH)

all: build-$(ARCH)

build-$(ARCH): $(KERNEL_ISO)

# asm to obj
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(NASM) $(ASM_FLAGS) $< -o $@
	@echo "$(COLOUR_GREEN)Assembled $< to $@ $(COLOR_END)"

# inc to obj
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.inc
	@mkdir -p $(dir $@)
	$(NASM) $(ASM_FLAGS) $< -o $@
	@echo "$(COLOUR_GREEN)Assembled $< to $@ $(COLOR_END)"


# c to obj
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "$(COLOUR_GREEN)Assembled $< to $@ $(COLOR_END)"

# link kernel bin
$(KERNEL_BIN): $(ASM_OBJS) $(C_OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LD_FLAGS) -o $@ $^

# gen iso
$(KERNEL_ISO): $(KERNEL_BIN)
	@mkdir -p $(dir $(BOOT_BIN))
	cp $< $(BOOT_BIN)
	@if [ -z "$(GRUB_MKRESCUE)" ]; then \
		echo "Error: grub-mkrescue or grub2-mkrescue is required to build the ISO." >&2; \
		exit 1; \
	fi
	$(GRUB_MKRESCUE) -d /usr/lib/grub/i386-pc -o $@ $(TARGET_DIR)/iso

clean:
	rm -rf build dist
