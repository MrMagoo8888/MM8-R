// Mostly stole from MM8-OS

#include "stddef.h"
#include "stdbool.h"
#include "graphics.h"
#include "font.h"
#include "liballoc.h"
#include "stdarg.h"
#include "vbe.h"
#include "screenDefs.h"

static const char g_HexChars[] = "0123456789abcdef";

// color Palette (0-15) mapped to 32-bit RGB
static const uint32_t vga_colors[16] = {
    0x00000000, // 0: Black
    0x000000AA, // 1: Blue
    0x0000AA00, // 2: Green
    0x0000AAAA, // 3: Cyan
    0x00AA0000, // 4: Red
    0x00AA00AA, // 5: Magenta
    0x00AA5500, // 6: Brown
    0x00AAAAAA, // 7: Light Gray
    0x00555555, // 8: Dark Gray
    0x005555FF, // 9: Light Blue
    0x0055FF55, // 10: Light Green
    0x0055FFFF, // 11: Light Cyan
    0x00FF5555, // 12: Light Red
    0x00FF55FF, // 13: Light Magenta
    0x00FFFF55, // 14: Yellow
    0x00FFFFFF  // 15: White
};

bool g_ConsoleAutoSwap = true;
uint32_t g_ConsoleDelay = 0;

// Shadow buffer for text (since we can't read back from VBE easily)
// Assuming 80x25 standard text resolution for logic
// static uint8_t g_ShadowBuffer[SCREEN_WIDTH * SCREEN_HEIGHT * 2];
static uint8_t* g_ShadowBuffer = NULL;
uint8_t* g_ScreenBuffer = NULL;

int g_ScreenX = 0, g_ScreenY = 0;
int g_ConsoleWidth = 80;
int g_ConsoleHeight = 25;
int g_FontScale = 2; // Scale 1x by default

// --- Scrollback Buffer ---
// char scrollback_buffer[SCROLLBACK_LINES][SCREEN_WIDTH];
char* scrollback_buffer = NULL; // Flat buffer: SCROLLBACK_LINES * g_ConsoleWidth
int scrollback_start = 0;
int scrollback_count = 0;
int scrollback_view = 0;

// - live screen backup
// static uint8_t live_screen_backup[SCREEN_HEIGHT * SCREEN_WIDTH * 2];
static uint8_t* live_screen_backup = NULL;
static bool in_scrollback_mode = false;


uint8_t getcolor(int x, int y) {
    if (!g_ScreenBuffer) return DEFAULT_COLOR;
    return g_ScreenBuffer[2 * (y * g_ConsoleWidth + x) + 1];
}


static void draw_char_at(int x, int y, char c, uint8_t color) {
    if (!g_vbe_screen) return;

    uint32_t fg = vga_colors[color & 0x0F];
    uint32_t bg = vga_colors[(color >> 4) & 0x0F];

    // Get font data (offset by 32 because our font starts at space)
    const uint8_t* glyph = (c >= 32 && c <= 127) ? font8x8_basic[c - 32] : font8x8_basic[0];

    int screen_x = x * 8 * g_FontScale;
    int screen_y = y * 8 * g_FontScale;

    // Draw 8x8 pixels
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            // Check if the bit is set in the font bitmap
            // Bit 0 is the rightmost pixel, Bit 7 is leftmost
            bool pixel_on = (glyph[row] >> (7 - col)) & 1;
            uint32_t draw_color = pixel_on ? fg : bg;
            
            // Draw scaled pixel
            for (int sy = 0; sy < g_FontScale; sy++) {
                for (int sx = 0; sx < g_FontScale; sx++) {
                    draw_pixel(screen_x + col * g_FontScale + sx, 
                               screen_y + row * g_FontScale + sy, 
                               draw_color);
                }
            }
        }
    }
}


void putchr(int x, int y, char c)
{
    if (g_ScreenBuffer) {
        g_ScreenBuffer[2 * (y * g_ConsoleWidth + x)] = c;
    }
    
    // Draw to VBE
    draw_char_at(x, y, c, getcolor(x, y));
}

char getchr(int x, int y)
{
    if (!g_ScreenBuffer) return 0;
    return g_ScreenBuffer[2 * (y * g_ConsoleWidth + x)];
}

void clrscr()
{
    if (g_ScreenBuffer) {
        // Clear the shadow buffer
        for (int y = 0; y < g_ConsoleHeight; y++)
            for (int x = 0; x < g_ConsoleWidth; x++)
            {
                g_ScreenBuffer[2 * (y * g_ConsoleWidth + x)] = '\0';
                g_ScreenBuffer[2 * (y * g_ConsoleWidth + x) + 1] = DEFAULT_COLOR;
            }
    }

    g_ScreenX = 0;
    g_ScreenY = 0;
    
    // Clear the screen using the graphics abstraction (respects double buffering)
    if (g_vbe_screen) {
        graphics_clear_buffer(0x00000000);
        if (g_DoubleBufferEnabled) graphics_swap_buffer();
    }
}

//scroolback

void putc(char c)
{
    switch (c)
    {
        case '\n':
            g_ScreenX = 0;
            g_ScreenY++;
            break;

        case '\b':
            if (g_ScreenX > 0) {
                g_ScreenX--;
                putchr(g_ScreenX, g_ScreenY, ' ');
            } else if (g_ScreenY > 0) {
                g_ScreenY--;
                g_ScreenX = g_ConsoleWidth - 1;
                putchr(g_ScreenX, g_ScreenY, ' ');
            }
            break;
    
        case '\t':
            for (int i = 0; i < 4 - (g_ScreenX % 4); i++)
                putc(' ');
            break;

        case '\r':
            g_ScreenX = 0;
            break;

        default:
            putchr(g_ScreenX, g_ScreenY, c);
            g_ScreenX++;
            break;
    }

    if (g_ScreenX >= g_ConsoleWidth)
    {
        g_ScreenY++;
        g_ScreenX = 0;
    }
    //if (g_ScreenY >= g_ConsoleHeight)
        //scrollback(1);

    //setcursor(g_ScreenX, g_ScreenY);

    //if (g_ConsoleDelay > 0)
        //sleep_ms(g_ConsoleDelay);

    if (g_ConsoleAutoSwap && g_DoubleBufferEnabled)
        graphics_swap_buffer();
}