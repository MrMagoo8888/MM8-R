The docs dir is for general systemdocumentation and perFile documentation,

Main rules: 
    - When you create or update a file note what happened as a doc (read on for explaination)
    - Include changes and author/dev
    - follow file structure of real code

Current tree:

.
├── build
├── build_scripts
│   ├── config.mk
│   ├── generate_isrs.sh
│   └── toolchain.mk
├── dist
│   └── x86_64
│       ├── kernel.bin
│       └── kernel.iso
├── docs
│   └── overview.md
├── Makefile
├── qlog.txt
├── r.sh
├── src
│   ├── arch
│   │   └── x86_64
│   │       ├── boot
│   │       │   ├── header.asm
│   │       │   ├── main64.asm
│   │       │   └── main.asm
│   │       ├── drivers
│   │       │   ├── ahci.c
│   │       │   └── ahci.h
│   │       └── interrupts
│   │           ├── apic.c
│   │           ├── apic.h
│   │           ├── binary.h
│   │           ├── gdt.c
│   │           ├── gdt.h
│   │           ├── idt_asm.asm
│   │           ├── idt.c
│   │           ├── idt.h
│   │           ├── isrs_gen_asm.inc
│   │           └── isrs_gen.c
│   └── kernel
│       ├── commands
│       ├── drivers
│       │   ├── keyboard.c
│       │   ├── pcie.c
│       │   ├── pic.c
│       │   └── vbe.c
│       ├── fs
│       ├── include
│       │   ├── basicShell.h
│       │   ├── font.h
│       │   ├── graphics.h
│       │   ├── heap.h
│       │   ├── io.h
│       │   ├── keyboard.h
│       │   ├── liballoc.h
│       │   ├── memory.h
│       │   ├── minmax.h
│       │   ├── pcie.h
│       │   ├── pic.h
│       │   ├── screenDefs.h
│       │   ├── splash.h
│       │   ├── stdarg.h
│       │   ├── stdbool.h
│       │   ├── stddef.h
│       │   ├── stdint.h
│       │   ├── stdio.h
│       │   ├── string.h
│       │   └── vbe.h
│       ├── libs
│       │   ├── basicShell.c
│       │   ├── graphics.c
│       │   ├── screenDefs.c
│       │   ├── stdio.c
│       │   └── string.c
│       ├── main.c
│       ├── mem
│       │   ├── heap.c
│       │   ├── liballoc.c
│       │   └── memory.c
│       └── misc
│           └── splash.c
└── targets
    └── x86_64
        ├── iso
        │   ├── boot
        │   │   ├── grub
        │   │   │   └── grub.cfg
        │   │   └── kernel.bin
        │   └── root
        │       ├── bin
        │       ├── home
        │       ├── lib
        │       ├── opt
        │       ├── sbin
        │       ├── tmp
        │       └── usr
        └── linker.ld
