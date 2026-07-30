ENTRY(start)

SECTIONS
{
    /* start mapping kernel sections at 1 MiB physical address line */
    . = 1M;

    /* Multiboot2 header block must be located at absolute front */
    .boot :
    {
        KEEP(*(.multiboot_header))
    }

    /* executable code section */
    .text :
    {
        *(.text)
    }

    /* read-only global strings und constants */
    .rodata :
    {
        *(.rodata)
    }

    /* initialised global und static variables */
    .data :
    {
        *(.data)
    }

    /* uninitialised global data und stack structures space allocation */
    .bss :
    {
        *(.bss)
    }

    /* heap stuff */
    /* force 4096-byte alignment to clear final page boundary safely */
    . = ALIGN(4096);
    __end = .;
}
