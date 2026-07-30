#include "stdint.h"
#include "vbe.h"

static uint32_t* fb_addr = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint32_t fb_pitch = 0;

void vbe_init(uint64_t multiboot_addr) {
    struct multiboot_tag* tag = (struct multiboot_tag*)(multiboot_addr + 8);

    while (tag->type != 0) {
        if (tag->type == 8) {
            struct multiboot_tag_framebuffer* fb_tag = (struct multiboot_tag_framebuffer*)tag;
            
            fb_addr = (uint32_t*)fb_tag->framebuffer_addr;
            fb_width = fb_tag->framebuffer_width;
            fb_height = fb_tag->framebuffer_height;
            fb_pitch = fb_tag->framebuffer_pitch;

            // Old test, works, code retired but may be brought backfor
            uint32_t total_pixels = fb_width * fb_height;
            for (uint32_t i = 0; i < total_pixels; i++) {
                fb_addr[i] = 0x00FF0000; // Bright Red
            }
            return; // Exit early once found
        }
        tag = (struct multiboot_tag*)((uint64_t)tag + ((tag->size + 7) & ~7));
    }
}

void vbe_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb_addr || x >= fb_width || y >= fb_height) return;
    
    // Pitch / 4 converts byte offset to uint32_t pixel array index
    uint32_t index = x + y * (fb_pitch / 4);
    fb_addr[index] = color;
}
