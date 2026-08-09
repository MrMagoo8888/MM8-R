#include "vbe.h"
#include "memory.h"
#include "liballoc.h"

void kernel_main(uint64_t multiboot_addr) {
    // init graphics buffer
    vbe_init(multiboot_addr);

    // initialize a simple page-backed allocator for liballoc
    memory_init(multiboot_addr);

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

    while (1) {
    }
}
