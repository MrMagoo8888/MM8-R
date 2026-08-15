#include "vbe.h"
#include "memory.h"
#include "liballoc.h"
#include "stdio.h"
#include "../arch/x86_64/interrupts/gdt.h"
#include "../arch/x86_64/interrupts/idt.h"

/*struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};  */

void kernel_main(uint64_t multiboot_addr, uint64_t magic) {

    if (magic != 0x36D76289) {
        return;
    }

    // init with real multiboot addr
    x86_64_GDT_Initialize();
    memory_init(multiboot_addr);
    x86_64_IDT_Initialize();
    vbe_init(multiboot_addr);

    // parse mbi total size
    uint32_t mbi_size = *(volatile uint32_t*)multiboot_addr;

    // loop tags starting 8 bytes after the base
    uint64_t current_tag_addr = multiboot_addr + 8;
    void* acpi_rsdp = NULL;

    while (current_tag_addr < (multiboot_addr + mbi_size)) {
        struct multiboot_tag* tag = (struct multiboot_tag*)current_tag_addr;

        if (tag->type == 0) {
            break;
        }

        if (tag->type == 15) {     // ACPI 2.0+ RSDP
            acpi_rsdp = (void*)(current_tag_addr + 8);
        }
        else if (tag->type == 14) {     // ACPI 1.0 fallback
            if (!acpi_rsdp) {
                acpi_rsdp = (void*)(current_tag_addr + 8);
            }
        }

        // move to next tag
        current_tag_addr += ((tag->size + 7) & ~7);
    }

    if (acpi_rsdp) {
        // init pcie
    }

    // test allocations so allocator exercised
    void *a = malloc(32);
    void *b = malloc(64);
    if (a && b) {
        ((char *)a)[0] = 'A';
        ((char *)b)[0] = 'B';
        free(a);
        free(b);
    }

    // draw a bright red pixel X=100, Y=100
    // Color hex format: 0x00RRGGBB
    vbe_put_pixel(100, 100, 0x000000FF);

    putchr(15, 15, "a");

    while (1) {
    }
}
