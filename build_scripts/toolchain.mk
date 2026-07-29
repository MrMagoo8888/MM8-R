# Define toolchain paths
TOOLCHAIN_I686   = $(abspath toolchain/i686-elf)
TOOLCHAIN_X86_64 = $(abspath toolchain/x86_64-elf)


export PATH := $(TOOLCHAIN_I686)/bin:$(TOOLCHAIN_X86_64)/bin:$(PATH)

#sSource and build dirs seperated by architecture (big fancey word haha)
BINUTILS_SRC       = toolchain/binutils-$(BINUTILS_VERSION)
BINUTILS_BUILD_32  = toolchain/binutils-build-i686-$(BINUTILS_VERSION)
BINUTILS_BUILD_64  = toolchain/binutils-build-x86_64-$(BINUTILS_VERSION)

GCC_SRC            = toolchain/gcc-$(GCC_VERSION)
GCC_BUILD_32       = toolchain/gcc-build-i686-$(GCC_VERSION)
GCC_BUILD_64       = toolchain/gcc-build-x86_64-$(GCC_VERSION)

.PHONY: toolchain_all toolchain_32 toolchain_64 clean-toolchain clean-toolchain-all

# Main entry point to build everything sequentially
toolchain_all: toolchain_32 toolchain_64

toolchain_32: $(TOOLCHAIN_I686)/bin/i686-elf-gcc
toolchain_64: $(TOOLCHAIN_X86_64)/bin/x86_64-elf-gcc

# share source downloads
$(BINUTILS_SRC).tar.xz:
	mkdir -p toolchain 
	cd toolchain && wget $(BINUTILS_URL)

$(GCC_SRC).tar.xz:
	mkdir -p toolchain
	cd toolchain && wget $(GCC_URL)

# 
#i686-elf build Rules
# 
$(TOOLCHAIN_I686)/bin/i686-elf-ld: $(BINUTILS_SRC).tar.xz
	cd toolchain && tar -xf binutils-$(BINUTILS_VERSION).tar.xz
	mkdir -p $(BINUTILS_BUILD_32)
	cd $(BINUTILS_BUILD_32) && CFLAGS="-Wno-error -fpermissive" CXXFLAGS="-Wno-error -fpermissive" ASMFLAGS= CC= CXX= LD= ASM= LINKFLAGS= LIBS= ../binutils-$(BINUTILS_VERSION)/configure \
		--prefix="$(TOOLCHAIN_I686)"	\
		--target=i686-elf				\
		--with-sysroot					\
		--disable-nls					\
		--disable-werror
	$(MAKE) -j8 -C $(BINUTILS_BUILD_32)
	$(MAKE) -C $(BINUTILS_BUILD_32) install

$(TOOLCHAIN_I686)/bin/i686-elf-gcc: $(TOOLCHAIN_I686)/bin/i686-elf-ld $(GCC_SRC).tar.xz
	cd toolchain && tar -xf gcc-$(GCC_VERSION).tar.xz
	cd $(GCC_SRC) && ./contrib/download_prerequisites
	mkdir -p $(GCC_BUILD_32)
	cd $(GCC_BUILD_32) && CFLAGS="-Wno-error -fpermissive" CXXFLAGS="-Wno-error -fpermissive" ASMFLAGS= CC= CXX= LD= ASM= LINKFLAGS= LIBS= ../gcc-$(GCC_VERSION)/configure \
		--prefix="$(TOOLCHAIN_I686)" 	\
		--target=i686-elf				\
		--disable-nls					\
		--enable-languages=c,c++		\
		--without-headers
	$(MAKE) -j8 -C $(GCC_BUILD_32) all-gcc all-target-libgcc
	$(MAKE) -C $(GCC_BUILD_32) install-gcc install-target-libgcc

# 
#x86_64-elf Build Rules
# 
$(TOOLCHAIN_X86_64)/bin/x86_64-elf-ld: $(BINUTILS_SRC).tar.xz
	cd toolchain && tar -xf binutils-$(BINUTILS_VERSION).tar.xz
	mkdir -p $(BINUTILS_BUILD_64)
	cd $(BINUTILS_BUILD_64) && CFLAGS="-Wno-error -fpermissive" CXXFLAGS="-Wno-error -fpermissive" ASMFLAGS= CC= CXX= LD= ASM= LINKFLAGS= LIBS= ../binutils-$(BINUTILS_VERSION)/configure \
		--prefix="$(TOOLCHAIN_X86_64)"	\
		--target=x86_64-elf				\
		--with-sysroot					\
		--disable-nls					\
		--disable-werror
	$(MAKE) -j8 -C $(BINUTILS_BUILD_64)
	$(MAKE) -C $(BINUTILS_BUILD_64) install

$(TOOLCHAIN_X86_64)/bin/x86_64-elf-gcc: $(TOOLCHAIN_X86_64)/bin/x86_64-elf-ld $(GCC_SRC).tar.xz
	cd toolchain && tar -xf gcc-$(GCC_VERSION).tar.xz
	cd $(GCC_SRC) && ./contrib/download_prerequisites
	mkdir -p $(GCC_BUILD_64)
	cd $(GCC_BUILD_64) && CFLAGS="-Wno-error -fpermissive" CXXFLAGS="-Wno-error -fpermissive" ASMFLAGS= CC= CXX= LD= ASM= LINKFLAGS= LIBS= ../gcc-$(GCC_VERSION)/configure \
		--prefix="$(TOOLCHAIN_X86_64)" 	\
		--target=x86_64-elf				\
		--disable-nls					\
		--enable-languages=c,c++		\
		--without-headers
	$(MAKE) -j8 -C $(GCC_BUILD_64) all-gcc all-target-libgcc
	$(MAKE) -C $(GCC_BUILD_64) install-gcc install-target-libgcc

#
# cleaning Operations
# 
clean-toolchain:
	rm -rf $(GCC_BUILD_32) $(GCC_BUILD_64) $(GCC_SRC) $(BINUTILS_BUILD_32) $(BINUTILS_BUILD_64) $(BINUTILS_SRC)

clean-toolchain-all:
	rm -rf toolchain/*
