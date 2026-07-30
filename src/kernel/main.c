#include "vbe.h"

void kernel_main(uint64_t multiboot_addr) {
    // init graphics buffer
    vbe_init(multiboot_addr);

    // draw a bright red pixel X=100, Y=100
    // Color hex format: 0x00RRGGBB
    vbe_put_pixel(100, 100, 0x000000FF); 

    while(1);
}
