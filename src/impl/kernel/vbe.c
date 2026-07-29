#include "vbe.h"
#include "stdint.h"

// Keep framebuffer details private to this file
static uint32_t* fb_addr = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint32_t fb_pitch = 0; // Number of bytes per scanline

void vbe_init(uint64_t multiboot_addr) {
    // First 4 bytes contain total size of the multiboot structure
    uint32_t total_size = *(uint32_t*)multiboot_addr;
    
    // First tag starts 8 bytes after the base address
    struct multiboot_tag* tag = (struct multiboot_tag*)(multiboot_addr + 8);

    // Loop through tags until we hit an end tag (type 0)
    while (tag->type != 0) {
        if (tag->type == 8) { // Type 8 is Framebuffer info
            struct multiboot_tag_framebuffer* fb_tag = (struct multiboot_tag_framebuffer*)tag;
            
            fb_addr = (uint32_t*)fb_tag->framebuffer_addr;
            fb_width = fb_tag->framebuffer_width;
            fb_height = fb_tag->framebuffer_height;
            fb_pitch = fb_tag->framebuffer_pitch;
            break;
        }
        
        // Advance to next tag (aligned to 8 bytes boundary)
        tag = (struct multiboot_tag*)((uint64_t)tag + ((tag->size + 7) & ~7));
    }
}

void vbe_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb_addr || x >= fb_width || y >= fb_height) return;
    
    // Pitch / 4 converts byte offset to uint32_t pixel array index
    uint32_t index = x + y * (fb_pitch / 4);
    fb_addr[index] = color;
}
