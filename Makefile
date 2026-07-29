include build_scripts/config.mk
include build_scripts/toolchain.mk

# ------------------------------------------------------------------------------
# Abstraction & Variables
# ------------------------------------------------------------------------------
ARCH        := x86_64
SRC_DIR     := src/impl/$(ARCH)
BUILD_DIR   := build/$(ARCH)
DIST_DIR    := dist/$(ARCH)
TARGET_DIR  := targets/$(ARCH)

# Toolchain setup
CROSS_COMPILE ?= $(ARCH)-elf-
NASM          ?= nasm
LD            := $(CROSS_COMPILE)ld
GRUB_MKRESCUE ?= grub-mkrescue

ASM_FLAGS   := -f elf64
LD_FLAGS    := -n -T $(TARGET_DIR)/linker.ld

# Clean, robust recursive wildcard implementation
rwildcard = $(wildcard $1/$2) $(foreach d,$(wildcard $1/*),$(call rwildcard,$d,$2))

# Discover files - strip whitespace to prevent path matching bugs
ASM_SRCS    := $(strip $(call rwildcard,$(SRC_DIR),*.asm))
ASM_OBJS    := $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_SRCS))

KERNEL_BIN  := $(DIST_DIR)/kernel.bin
KERNEL_ISO  := $(DIST_DIR)/kernel.iso
BOOT_BIN    := $(TARGET_DIR)/iso/boot/kernel.bin

# Sanity Check: Fail early if no assembly sources are located
ifeq ($(ASM_SRCS),)
$(error No .asm source files found in '$(SRC_DIR)'. Please check directory path and file extensions)
endif

# ------------------------------------------------------------------------------
# Targets & Rules
# ------------------------------------------------------------------------------
.DEFAULT_GOAL := all

.PHONY: all clean build-$(ARCH)

all: build-$(ARCH)

build-$(ARCH): $(KERNEL_ISO)

# Compile Assembly sources to Object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(NASM) $(ASM_FLAGS) $< -o $@

# Link kernel binary
$(KERNEL_BIN): $(ASM_OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LD_FLAGS) -o $@ $^

# Generate ISO image
$(KERNEL_ISO): $(KERNEL_BIN)
	@mkdir -p $(dir $(BOOT_BIN))
	cp $< $(BOOT_BIN)
	$(GRUB_MKRESCUE) /usr/lib/grub/i386-pc -o $@ $(TARGET_DIR)/iso

clean:
	rm -rf build dist