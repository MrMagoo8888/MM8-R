#ifndef VBE_H
#define VBE_H

#include "stdint.h"

// Multiboot2 basic tag header
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

// Multiboot2 Framebuffer Info tag (Type 8)
struct multiboot_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
};

// Public initialization function
void vbe_init(uint64_t multiboot_addr);
void vbe_put_pixel(uint32_t x, uint32_t y, uint32_t color);

#endif
