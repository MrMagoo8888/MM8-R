ARCH        ?= x86_64
SRC_DIR     := src
BUILD_DIR   := build/$(ARCH)
DIST_DIR    := dist/$(ARCH)
TARGET_DIR  := targets/$(ARCH)

TOOLCHAIN_BIN := /home/mm8/code/os/mm8/MM8-R/toolchain/x86_64-elf/bin
NASM        := nasm
LD          := $(TOOLCHAIN_BIN)/x86_64-elf-ld
CC          := $(TOOLCHAIN_BIN)/x86_64-elf-gcc

CFLAGS      := -m64 -ffreestanding -fno-builtin -O2 -Wall -Wextra -I$(SRC_DIR)/kernel/include -I$(SRC_DIR)/arch/x86_64/include
ASM_FLAGS   := -f elf64
LD_FLAGS    := -n -T $(TARGET_DIR)/linker.ld

rwildcard    = $(foreach d,$(wildcard $(1)/*),$(call rwildcard,$(d),$(2)) $(filter $(subst *,%,$(2)),$(d)))


# Discover files 
ASM_SRCS    := $(strip $(call rwildcard,$(SRC_DIR)/arch,*.asm))
ASM_OBJS    := $(patsubst src/%.asm,build/$(ARCH)/%.o,$(ASM_SRCS))

C_SRCS      := $(strip $(call rwildcard,$(SRC_DIR)/,*.c))
C_OBJS      := $(patsubst src/%.c,build/$(ARCH)/%.o,$(C_SRCS))

KERNEL_BIN  := $(DIST_DIR)/kernel.bin
KERNEL_ISO  := $(DIST_DIR)/kernel.iso
BOOT_BIN    := $(TARGET_DIR)/iso/boot/kernel.bin

# Sanity Check: Fail early if no assembly sources are located
ifeq ($(ASM_SRCS),)
$(error No .asm source files found in '$(SRC_DIR)'. Please check directory path and file extensions)
endif


# Targets und Rules
.DEFAULT_GOAL := all

.PHONY: all clean build-$(ARCH)

all: build-$(ARCH)

build-$(ARCH): $(KERNEL_ISO)

# Compile Assembly sources to Object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(NASM) $(ASM_FLAGS) $< -o $@

# Compile C sources to Object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link kernel binary 
$(KERNEL_BIN): $(ASM_OBJS) $(C_OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LD_FLAGS) -o $@ $^

# Generate ISO image
$(KERNEL_ISO): $(KERNEL_BIN)
	@mkdir -p $(dir $(BOOT_BIN))
	cp $< $(BOOT_BIN)
	grub-mkrescue -d /usr/lib/grub/i386-pc -o $@ $(TARGET_DIR)/iso

clean:
	rm -rf build dist
