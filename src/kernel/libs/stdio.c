#include <stdio.h>
//#include <arch/i686/io.h>
#include <stdint.h>
#include "stdio.h"
#include "stdbool.h"
#include <stdarg.h>
#include "memory.h"
#include "vbe.h"
#include "graphics.h"
#include "font.h"
#include "heap.h"
//#include "time.h"

static const char g_HexChars[] = "0123456789abcdef";

// VGA Color Palette (0-15) mapped to 32-bit RGB
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

int32_t g_ScreenX = 0, g_ScreenY = 0;
int32_t g_ConsoleWidth = 80;
int32_t g_ConsoleHeight = 25;
int32_t g_FontScale = 2; // Scale 1x by default

// --- Scrollback Buffer ---
// char scrollback_buffer[SCROLLBACK_LINES][SCREEN_WIDTH];
char* scrollback_buffer = NULL; // Flat buffer: SCROLLBACK_LINES * g_ConsoleWidth
int32_t scrollback_start = 0;
int32_t scrollback_count = 0;
int32_t scrollback_view = 0;

// --- Live Screen Backup ---
// static uint8_t live_screen_backup[SCREEN_HEIGHT * SCREEN_WIDTH * 2];
static uint8_t* live_screen_backup = NULL;
static bool in_scrollback_mode = false;

void console_initialize() {
    if (g_vbe_screen) {
        g_ConsoleWidth = g_vbe_screen->width / (8 * g_FontScale);
        g_ConsoleHeight = g_vbe_screen->height / (8 * g_FontScale);
    }

    // Allocate buffers based on dynamic size
    g_ShadowBuffer = (uint8_t*)malloc(g_ConsoleWidth * g_ConsoleHeight * 2);
    if (!g_ShadowBuffer) {
        kprintf("CRITICAL: Failed to allocate console shadow buffer!\n");
    }
    g_ScreenBuffer = g_ShadowBuffer;
    
    live_screen_backup = (uint8_t*)malloc(g_ConsoleWidth * g_ConsoleHeight * 2);
    if (!live_screen_backup) {
        kprintf("WARNING: Failed to allocate live screen backup.\n");
    }
    scrollback_buffer = (char*)malloc(SCROLLBACK_LINES * g_ConsoleWidth);
    if (!scrollback_buffer) {
        kprintf("WARNING: Failed to allocate scrollback buffer.\n");
    }

    // Clear buffers and give every cell a visible default color.
    if (g_ShadowBuffer) {
        size_t cell_count = (size_t)g_ConsoleWidth * g_ConsoleHeight;
        for (size_t cell = 0; cell < cell_count; cell++) {
            g_ShadowBuffer[cell * 2] = '\0';
            g_ShadowBuffer[cell * 2 + 1] = DEFAULT_COLOR;
        }
    }
    if (scrollback_buffer) memset(scrollback_buffer, 0, SCROLLBACK_LINES * g_ConsoleWidth);

    // Enable double buffering for the console
    graphics_set_double_buffering(true);
}

void console_set_font_scale(int32_t scale) {
    if (scale < 1) scale = 1;
    if (scale > 8) scale = 8; // Limit max scale

    // Free old buffers
    if (g_ShadowBuffer) free(g_ShadowBuffer);
    if (live_screen_backup) free(live_screen_backup);
    if (scrollback_buffer) free(scrollback_buffer);

    g_FontScale = scale;
    
    console_initialize();
    clrscr();
}

// Helper to draw a character to the VBE screen
static void draw_char_at(int32_t x, int32_t y, char c, uint8_t color) {
    if (!g_vbe_screen) return;

    uint32_t fg = vga_colors[color & 0x0F];
    uint32_t bg = vga_colors[(color >> 4) & 0x0F];

    // Get font data (offset by 32 because our font starts at space)
    const uint8_t* glyph = (c >= 32 && c <= 127) ? font8x8_basic[c - 32] : font8x8_basic[0];

    int32_t screen_x = x * 8 * g_FontScale;
    int32_t screen_y = y * 8 * g_FontScale;

    // Draw 8x8 pixels
    for (int32_t row = 0; row < 8; row++) {
        for (int32_t col = 0; col < 8; col++) {
            // Check if the bit is set in the font bitmap
            // Bit 0 is the rightmost pixel, Bit 7 is leftmost
            bool pixel_on = (glyph[row] >> (7 - col)) & 1;
            uint32_t draw_color = pixel_on ? fg : bg;
            
            // Draw scaled pixel
            for (int32_t sy = 0; sy < g_FontScale; sy++) {
                for (int32_t sx = 0; sx < g_FontScale; sx++) {
                    draw_pixel(screen_x + col * g_FontScale + sx, 
                               screen_y + row * g_FontScale + sy, 
                               draw_color);
                }
            }
        }
    }
}

void kputchr(int32_t x, int32_t y, char c)
{
    if (g_ScreenBuffer) {
        // Update shadow buffer
        g_ScreenBuffer[2 * (y * g_ConsoleWidth + x)] = c;
    }
    
    // Draw to VBE
    draw_char_at(x, y, c, kgetcolor(x, y));
}

void kputcolor(int32_t x, int32_t y, uint8_t color)
{
    if (!g_ScreenBuffer) return;
    // Update shadow buffer
    g_ScreenBuffer[2 * (y * g_ConsoleWidth + x) + 1] = color;
    
    // Draw to VBE
    draw_char_at(x, y, kgetchr(x, y), color);
}

char kgetchr(int32_t x, int32_t y)
{
    if (!g_ScreenBuffer) return 0;
    return g_ScreenBuffer[2 * (y * g_ConsoleWidth + x)];
}

uint8_t kgetcolor(int32_t x, int32_t y)
{
    if (!g_ScreenBuffer) return DEFAULT_COLOR;
    return g_ScreenBuffer[2 * (y * g_ConsoleWidth + x) + 1];
}

void setcursor(int32_t x, int32_t y)
{
    // Hardware cursor (VGA ports) doesn't work in VBE mode.
    // We would need to implement a software cursor (e.g., drawing an underscore).
    // For now, we just disable the port writes to avoid issues.
    
    // int32_t pos = y * SCREEN_WIDTH + x;
    // i686_outb(0x3D4, 0x0F);
    // i686_outb(0x3D5, (uint8_t)(pos & 0xFF));
    // i686_outb(0x3D4, 0x0E);
    // i686_outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void clrscr()
{
    if (g_ScreenBuffer) {
        // Clear the shadow buffer
        for (int32_t y = 0; y < g_ConsoleHeight; y++)
            for (int32_t x = 0; x < g_ConsoleWidth; x++)
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

void scrollback(int32_t lines)
{
    if (!scrollback_buffer) return;
    // Save scrolled-off lines to the scrollback buffer
    for (int32_t i = 0; i < lines; i++) {
        // Save the characters of the top line (y=0)
        int32_t row_idx = (scrollback_start + scrollback_count) % SCROLLBACK_LINES;
        for (int32_t x = 0; x < g_ConsoleWidth; x++) {
            scrollback_buffer[row_idx * g_ConsoleWidth + x] = kgetchr(x, i);
        }

        if (scrollback_count < SCROLLBACK_LINES) {
            scrollback_count++;
        } else {
            scrollback_start = (scrollback_start + 1) % SCROLLBACK_LINES;
        }
    }

    // Optimized Scrolling: Use memmove for instant scrolling
    
    // 1. Move Shadow Buffer (Text + Color)
    if (g_ScreenBuffer) {
        size_t line_size = g_ConsoleWidth * 2;
        size_t move_size = (g_ConsoleHeight - lines) * line_size;
        memmove(g_ScreenBuffer, g_ScreenBuffer + (lines * line_size), move_size);
    }

    // 2. Move VBE Framebuffer (Pixels)
    if (g_vbe_screen) {
        int32_t pixel_lines = lines * 8 * g_FontScale;
        size_t pitch = g_vbe_screen->pitch;
        size_t total_bytes = g_vbe_screen->height * pitch;
        size_t bytes_to_scroll = pixel_lines * pitch;
        
        void* target_buffer = (g_DoubleBufferEnabled && g_BackBuffer) 
            ? (void*)g_BackBuffer 
            : (void*)g_vbe_screen->physical_buffer;

        if (bytes_to_scroll < total_bytes) {
            memmove(target_buffer, 
                    (void*)((uintptr_t)target_buffer + bytes_to_scroll), 
                    total_bytes - bytes_to_scroll);
        }
    }

    // 3. Fast Clear Shadow Buffer bottom
    if (g_ScreenBuffer) {
        for (int32_t y = g_ConsoleHeight - lines; y < g_ConsoleHeight; y++) {
            for (int32_t x = 0; x < g_ConsoleWidth; x++) {
                int32_t base = 2 * (y * g_ConsoleWidth + x);
                g_ScreenBuffer[base] = '\0';
                g_ScreenBuffer[base + 1] = DEFAULT_COLOR;
            }
        }
    }

    // 4. Fast Clear VBE Framebuffer bottom (Pixels)
    if (g_vbe_screen) {
        int32_t pixel_lines = lines * 8 * g_FontScale;
        uint32_t bg_color = vga_colors[(DEFAULT_COLOR >> 4) & 0x0F];
        uint32_t pitch = g_vbe_screen->pitch;
        
        uint8_t* base_ptr = (g_DoubleBufferEnabled && g_BackBuffer)
            ? (uint8_t*)g_BackBuffer
            : (uint8_t*)g_vbe_screen->physical_buffer;

        for (int32_t y = g_vbe_screen->height - pixel_lines; y < g_vbe_screen->height; y++) {
            uint32_t* line = (uint32_t*)(base_ptr + y * pitch);
            if (bg_color == 0) {
                memset(line, 0, g_vbe_screen->width * 4);
            } else {
                for (int32_t x = 0; x < g_vbe_screen->width; x++) {
                    line[x] = bg_color;
                }
            }
        }
    }
    g_ScreenY -= lines;
    if (g_DoubleBufferEnabled) graphics_swap_buffer();
}

void console_refresh() {
    for (int32_t y = 0; y < g_ConsoleHeight; y++) {
        for (int32_t x = 0; x < g_ConsoleWidth; x++) {
            draw_char_at(x, y, kgetchr(x, y), kgetcolor(x, y));
        }
    }
    if (g_DoubleBufferEnabled) graphics_swap_buffer();
}

void refresh_screen_color()
{
    for (int32_t y = 0; y < g_ConsoleHeight; y++) {
        for (int32_t x = 0; x < g_ConsoleWidth; x++) {
            kputcolor(x, y, DEFAULT_COLOR);
        }
    }
    if (g_DoubleBufferEnabled) graphics_swap_buffer();
}

static void redraw_from_scrollback() {
    int32_t top_history_line = scrollback_count - scrollback_view;
    for (int32_t y = 0; y < g_ConsoleHeight; y++) {
        int32_t history_line_index = top_history_line - (g_ConsoleHeight - 1 - y);
        if (history_line_index >= 0 && history_line_index < scrollback_count) {
            int32_t buffer_idx = (scrollback_start + history_line_index) % SCROLLBACK_LINES;
            for (int32_t x = 0; x < g_ConsoleWidth; x++) {
                kputchr(x, y, scrollback_buffer[buffer_idx * g_ConsoleWidth + x]);
                kputcolor(x, y, DEFAULT_COLOR);
            }
        } else {
            // Clear lines that are beyond the history
            for (int32_t x = 0; x < g_ConsoleWidth; x++) {
                kputchr(x, y, ' ');
                kputcolor(x, y, DEFAULT_COLOR);
            }
        }
    }
    setcursor(0, g_ConsoleHeight - 1);
    if (g_DoubleBufferEnabled) graphics_swap_buffer();
}

void view_scrollback_up() {
    if (scrollback_count == 0) return;
    if (!live_screen_backup || !g_ScreenBuffer) return;
    if (!in_scrollback_mode) {
        memcpy(live_screen_backup, g_ScreenBuffer, g_ConsoleWidth * g_ConsoleHeight * 2);
        in_scrollback_mode = true;
    }
    scrollback_view++;
    if (scrollback_view > scrollback_count) {
        scrollback_view = scrollback_count;
    }
    redraw_from_scrollback();
}

void view_scrollback_down() {
    if (!in_scrollback_mode) return;
    if (!live_screen_backup || !g_ScreenBuffer) return;
    scrollback_view--;
    if (scrollback_view <= 0) {
        scrollback_view = 0;
        in_scrollback_mode = false;
        memcpy(g_ScreenBuffer, live_screen_backup, g_ConsoleWidth * g_ConsoleHeight * 2);
        console_refresh();
        setcursor(g_ScreenX, g_ScreenY);
        return;
    }
    redraw_from_scrollback();
}

void kputc(char c)
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
                kputchr(g_ScreenX, g_ScreenY, ' ');
            } else if (g_ScreenY > 0) {
                g_ScreenY--;
                g_ScreenX = g_ConsoleWidth - 1;
                kputchr(g_ScreenX, g_ScreenY, ' ');
            }
            break;
    
        case '\t':
            for (int32_t i = 0; i < 4 - (g_ScreenX % 4); i++)
                kputc(' ');
            break;

        case '\r':
            g_ScreenX = 0;
            break;

        default:
            kputchr(g_ScreenX, g_ScreenY, c);
            g_ScreenX++;
            break;
    }

    if (g_ScreenX >= g_ConsoleWidth)
    {
        g_ScreenY++;
        g_ScreenX = 0;
    }
    if (g_ScreenY >= g_ConsoleHeight)
        scrollback(1);

    setcursor(g_ScreenX, g_ScreenY);

    /*if (g_ConsoleDelay > 0)
        sleep_ms(g_ConsoleDelay);   time needed*/

    if (g_ConsoleAutoSwap && g_DoubleBufferEnabled)
        graphics_swap_buffer();
}

void kputs(const char* str)
{
    bool prev = g_ConsoleAutoSwap;
    if (g_ConsoleDelay == 0) g_ConsoleAutoSwap = false;

    while(*str)
    {
        kputc(*str);
        str++;
    }

    g_ConsoleAutoSwap = prev;
    if (g_ConsoleAutoSwap && g_DoubleBufferEnabled)
        graphics_swap_buffer();
}

void printf_unsigned(uint64_t number, int32_t radix, int32_t width, char padding, bool uppercase)
{
    char buffer[64];
    int32_t pos = 0;
    const char* hexChars = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

    // convert number to ASCII
    do 
    {
        uint64_t rem = number % radix;
        number /= radix;
        buffer[pos++] = hexChars[rem];
    } while (number > 0);

    // Print padding
    while (pos < width)
    {
        kputc(padding);
        width--;
    }

    // print number in reverse order
    while (--pos >= 0)
        kputc(buffer[pos]);
}

void printf_signed(int64_t number, int32_t radix, int32_t width, char padding, bool uppercase)
{
    if (number < 0)
    {
        if (padding == '0')
        {
            kputc('-');
            printf_unsigned(-number, radix, width > 0 ? width - 1 : 0, padding, uppercase);
        }
        else
        {
            // For space padding, we need to print spaces before the negative sign
            char buffer[32];
            int32_t pos = 0;
            uint64_t abs_val = -number;
            const char* hexChars = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
            
            do {
                buffer[pos++] = hexChars[abs_val % radix];
                abs_val /= radix;
            } while (abs_val > 0);

            // padding = width - (digits + sign)
            while (pos + 1 < width)
            {
                kputc(padding);
                width--;
            }

            kputc('-');
            while (--pos >= 0)
                kputc(buffer[pos]);
        }
    }
    else printf_unsigned(number, radix, width, padding, uppercase);
}

#define PRINTF_STATE_NORMAL         0
#define PRINTF_STATE_LENGTH         1
#define PRINTF_STATE_LENGTH_SHORT   2
#define PRINTF_STATE_LENGTH_LONG    3
#define PRINTF_STATE_SPEC           4
#define PRINTF_STATE_FLAGS          5
#define PRINTF_STATE_WIDTH          6

#define PRINTF_LENGTH_DEFAULT       0
#define PRINTF_LENGTH_SHORT_SHORT   1
#define PRINTF_LENGTH_SHORT         2
#define PRINTF_LENGTH_LONG          3
#define PRINTF_LENGTH_LONG_LONG     4

void kprintf(const char* fmt, ...)
{
    bool prev = g_ConsoleAutoSwap;
    if (g_ConsoleDelay == 0) g_ConsoleAutoSwap = false;

    va_list args;
    va_start(args, fmt);

    int32_t state = PRINTF_STATE_NORMAL;
    int32_t length = PRINTF_LENGTH_DEFAULT;
    int32_t radix = 10;
    bool sign = false;
    bool number = false;
    bool uppercase = false;
    int32_t width = 0;
    char padding = ' ';

    while (*fmt)
    {
        switch (state)
        {
            case PRINTF_STATE_NORMAL:
                switch (*fmt)
                {
                    case '%':   state = PRINTF_STATE_FLAGS;
                                width = 0;
                                padding = ' ';
                                uppercase = false;
                                break;
                    default:    kputc(*fmt);
                                break;
                }
                break;

            case PRINTF_STATE_FLAGS:
                if (*fmt == '0')
                {
                    padding = '0';
                    state = PRINTF_STATE_WIDTH;
                }
                else
                {
                    state = PRINTF_STATE_WIDTH;
                    goto PRINTF_STATE_WIDTH_;
                }
                break;

            case PRINTF_STATE_WIDTH:
            PRINTF_STATE_WIDTH_:
                if (*fmt >= '0' && *fmt <= '9')
                {
                    width = width * 10 + (*fmt - '0');
                }
                else
                {
                    state = PRINTF_STATE_LENGTH;
                    goto PRINTF_STATE_LENGTH_;
                }
                break;

            case PRINTF_STATE_LENGTH:
            PRINTF_STATE_LENGTH_:
                switch (*fmt)
                {
                    case 'h':   length = PRINTF_LENGTH_SHORT;
                                state = PRINTF_STATE_LENGTH_SHORT;
                                break;

                    case 'l':   length = PRINTF_LENGTH_LONG;
                                state = PRINTF_STATE_LENGTH_LONG;
                                break;

                    default:    state = PRINTF_STATE_SPEC;
                                goto PRINTF_STATE_SPEC_;
                }
                break;

            case PRINTF_STATE_LENGTH_SHORT:
                if (*fmt == 'h')
                {
                    length = PRINTF_LENGTH_SHORT_SHORT;
                    state = PRINTF_STATE_SPEC;
                }
                else goto PRINTF_STATE_SPEC_;
                break;

            case PRINTF_STATE_LENGTH_LONG:
                if (*fmt == 'l')
                {
                    length = PRINTF_LENGTH_LONG_LONG;
                    state = PRINTF_STATE_SPEC;
                }
                else goto PRINTF_STATE_SPEC_;
                break;

            case PRINTF_STATE_SPEC:
            PRINTF_STATE_SPEC_:
                switch (*fmt)
                {
                    case 'c':   kputc((char)va_arg(args, int32_t));
                                break;

                    case 's':   
                                kputs(va_arg(args, const char*));
                                break;

                    case '%':   kputc('%');
                                break;

                    case 'd':
                    case 'i':   radix = 10; sign = true; number = true;
                                break;

                    case 'u':   radix = 10; sign = false; number = true;
                                break;

                    case 'X':   radix = 16; sign = false; number = true; uppercase = true;
                                break;

                    case 'x':
                    case 'p':   radix = 16; sign = false; number = true;
                                break;

                    case 'o':   radix = 8; sign = false; number = true;
                                break;

                    // ignore invalid spec
                    default:    break;
                }

                if (number)
                {
                    if (sign)
                    {
                        switch (length)
                        {
                        case PRINTF_LENGTH_SHORT_SHORT:
                        case PRINTF_LENGTH_SHORT:
                        case PRINTF_LENGTH_DEFAULT:     printf_signed(va_arg(args, int32_t), radix, width, padding, uppercase);
                                                        break;

                        case PRINTF_LENGTH_LONG:        printf_signed(va_arg(args, int32_t), radix, width, padding, uppercase);
                                                        break;

                        case PRINTF_LENGTH_LONG_LONG:   printf_signed(va_arg(args, int64_t), radix, width, padding, uppercase);
                                                        break;
                        }
                    }
                    else
                    {
                        switch (length)
                        {
                        case PRINTF_LENGTH_SHORT_SHORT:
                        case PRINTF_LENGTH_SHORT:
                        case PRINTF_LENGTH_DEFAULT:     printf_unsigned(va_arg(args, int32_t), radix, width, padding, uppercase);
                                                        break;
                                                        
                        case PRINTF_LENGTH_LONG:        printf_unsigned(va_arg(args, int32_t), radix, width, padding, uppercase);
                                                        break;

                        case PRINTF_LENGTH_LONG_LONG:   printf_unsigned(va_arg(args, int64_t), radix, width, padding, uppercase);
                                                        break;
                        }
                    }
                }

                // reset state
                state = PRINTF_STATE_NORMAL;
                length = PRINTF_LENGTH_DEFAULT;
                radix = 10;
                sign = false;
                number = false;
                uppercase = false;
                width = 0;
                padding = ' ';
                break;
        }

        fmt++;
    }

    va_end(args);

    g_ConsoleAutoSwap = prev;
    if (g_ConsoleAutoSwap && g_DoubleBufferEnabled)
        graphics_swap_buffer();
}

void kprint_buffer(const char* msg, const void* buffer, uint32_t count)
{
    const uint8_t* u8Buffer = (const uint8_t*)buffer;
    
    kputs(msg);
    for (uint16_t i = 0; i < count; i++)
    {
        kputc(g_HexChars[u8Buffer[i] >> 4]);
        kputc(g_HexChars[u8Buffer[i] & 0xF]);
    }
    kputs("\n");
}

// SprintF implementation that writes to a string buffer instead of the console and i want to die now please and thank you
static void sprintf_unsigned(char** out, uint64_t number, int32_t radix, int32_t width, char padding, bool uppercase)
{
    char buffer[64];
    int32_t pos = 0;
    const char* hexChars = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

    do {
        buffer[pos++] = hexChars[number % radix];
        number /= radix;
    } while (number > 0);

    while (pos < width) {
        *(*out)++ = padding;
        width--;
    }

    while (--pos >= 0)
        *(*out)++ = buffer[pos];
}

static void sprintf_signed(char** out, int64_t number, int32_t radix, int32_t width, char padding, bool uppercase)
{
    if (number < 0) {
        *(*out)++ = '-';
        sprintf_unsigned(out, -number, radix, width > 0 ? width - 1 : 0, padding, uppercase);
    } else {
        sprintf_unsigned(out, number, radix, width, padding, uppercase);
    }
}

// Basic double to string for cJSON support
static void sprintf_float(char** out, double number, int32_t precision) {
    if (number < 0) {
        *(*out)++ = '-';
        number = -number;
    }

    int64_t int_part = (int64_t)number;
    sprintf_signed(out, int_part, 10, 0, ' ', false);

    if (precision > 0) {
        *(*out)++ = '.';
        double diff = number - (double)int_part;
        while (precision--) {
            diff *= 10;
            int32_t digit = (int32_t)diff;
            *(*out)++ = digit + '0';
            diff -= digit;
        }
    }
}

int32_t vsprintf(char* str, const char* fmt, va_list args) {
    char* start = str;
    int32_t state = PRINTF_STATE_NORMAL;
    int32_t length = PRINTF_LENGTH_DEFAULT;
    int32_t radix = 10;
    bool sign = false;
    bool number = false;
    bool uppercase = false;
    int32_t width = 0;
    char padding = ' ';

    while (*fmt) {
        switch (state) {
            case PRINTF_STATE_NORMAL:
                if (*fmt == '%') {
                    state = PRINTF_STATE_FLAGS;
                    width = 0; padding = ' '; uppercase = false;
                } else {
                    *str++ = *fmt;
                }
                break;

            case PRINTF_STATE_FLAGS:
                if (*fmt == '0') { padding = '0'; state = PRINTF_STATE_WIDTH; }
                else { state = PRINTF_STATE_WIDTH; goto WIDTH; }
                break;

            case PRINTF_STATE_WIDTH:
            WIDTH:
                if (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt - '0');
                else { state = PRINTF_STATE_LENGTH; goto LENGTH; }
                break;

            case PRINTF_STATE_LENGTH:
            LENGTH:
                if (*fmt == 'h') state = PRINTF_STATE_LENGTH_SHORT;
                else if (*fmt == 'l') state = PRINTF_STATE_LENGTH_LONG;
                else { state = PRINTF_STATE_SPEC; goto SPEC; }
                break;

            case PRINTF_STATE_SPEC:
            SPEC:
                number = false;
                switch (*fmt) {
                    case 'c': *str++ = (char)va_arg(args, int32_t); break;
                    case 's': {
                        char* s = va_arg(args, char*);
                        while (*s) *str++ = *s++;
                        break;
                    }
                    case 'd': case 'i': radix = 10; sign = true; number = true; break;
                    case 'u': radix = 10; sign = false; number = true; break;
                    case 'X': uppercase = true; radix = 16; sign = false; number = true; break;
                    case 'x': radix = 16; sign = false; number = true; break;
                    case 'f': case 'g':
                        sprintf_float(&str, va_arg(args, double), 6);
                        break;
                    case '%': *str++ = '%'; break;
                }

                if (number) {
                    if (sign) sprintf_signed(&str, va_arg(args, int64_t), radix, width, padding, uppercase);
                    else sprintf_unsigned(&str, va_arg(args, uint64_t), radix, width, padding, uppercase);
                }
                state = PRINTF_STATE_NORMAL;
                break;
        }
        fmt++;
    }
    *str = '\0';
    return str - start;
}

int32_t sprintf(char* str, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int32_t ret = vsprintf(str, format, args);
    va_end(args);
    return ret;
}

// STUB: A proper sscanf is a lot of work.
int32_t sscanf(const char* str, const char* format, ...) {
    return 0; // Pretend we parsed nothing.
}