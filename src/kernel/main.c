#include "vbe.h"
#include "memory.h"
#include "liballoc.h"
#include "stdio.h"
#include "../arch/x86_64/interrupts/gdt.h"
#include "../arch/x86_64/interrupts/idt.h"

void kernel_main(uint64_t multiboot_addr) {
    // init gdt
    x86_64_GDT_Initialize();

    memory_init(multiboot_addr);

    // init idt
    x86_64_IDT_Initialize();

    // init graphics buffer
    vbe_init(multiboot_addr);
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
