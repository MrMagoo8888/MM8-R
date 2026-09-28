#include "keyboard.h"
#include "io.h"
#include "pic.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdio.h"
#include "string.h"

typedef enum {
    INPUT_MODE_NONE,
    INPUT_MODE_GETS,  // Waiting for kgets()
    INPUT_MODE_GETCH, // Waiting for kgetch()
} InputMode;
static volatile InputMode g_CurrentInputMode = INPUT_MODE_NONE;

// --- Static variables for buffered input ---
#define INPUT_BUFFER_SIZE 256
static char g_InputBuffer[INPUT_BUFFER_SIZE];
static int32_t g_InputBufferIndex = 0;
static volatile bool g_InputLineReady = false;

// --- Static variables for single character input (kgetch) ---
static volatile int32_t g_CharBuffer = -1; // -1 means empty
static volatile bool g_CharReady = false;

static bool g_ShiftPressed = false;
static bool g_CtrlPressed = false;
static bool g_AltGrPressed = false;

// --- Command History ---
static char (*g_HistoryBuffer)[256] = NULL;
static int32_t* g_HistoryCount = NULL;
static int32_t* g_HistoryIndexPtr = NULL; // Pointer to main.c's g_HistoryIndex
static int32_t g_HistorySize = 0;
static int32_t g_HistoryNavIndex = -1; // How far back we are in history. -1 = not navigating. 0 = most recent.

extern uint8_t* g_ScreenBuffer;
extern int32_t g_ScreenX, g_ScreenY;

#define KEYBOARD_DATA 0x60
#define KEYBOARD_STATUS 0x64
#define KEYBOARD_BUFFER_SIZE 64

/*static const char keymap[128] = {
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4',
    [0x06] = '5', [0x07] = '6', [0x08] = '7', [0x09] = '8',
    [0x0A] = '9', [0x0B] = '0', [0x0C] = '-', [0x0D] = '=',
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r',
    [0x14] = 't', [0x15] = 'y', [0x16] = 'u', [0x17] = 'i',
    [0x18] = 'o', [0x19] = 'p', [0x1A] = '[', [0x1B] = ']',
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f',
    [0x22] = 'g', [0x23] = 'h', [0x24] = 'j', [0x25] = 'k',
    [0x26] = 'l', [0x27] = ';', [0x28] = '\'', [0x29] = '`',
    [0x2B] = '\\', [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c',
    [0x2F] = 'v', [0x30] = 'b', [0x31] = 'n', [0x32] = 'm',
    [0x33] = ',', [0x34] = '.', [0x35] = '/', [0x39] = ' ',
    [0x1C] = '\n', [0x0E] = '\b', [0x0F] = '\t'
};  */

// Scancode to ASCII mapping
static const char scancode_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', // 0-14
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, // 15-29
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '#', // 30-43
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, // 44-58
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 59-85
    '\\', // 86 (scancode 0x56)
};
// Scancode to ASCII mapping (shifted)
static const char scancode_ascii_shifted[128] = {
    0,  27, '!', '"', '\x9C', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', // 0-14, \x9C is £
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, // 15-29
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '@', '\xAA', 0, '~', // 30-43, \xAA is ¬
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, // 44-58
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 59-85
    '|', // 86 (scancode 0x56)
};
// Scancode to ASCII mapping (AltGr)
// Using CP437 character codes for accented letters
static const char scancode_ascii_altgr[128] = {
    0, 0, 0, 0, 0 /* € is not in CP437 */, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, '\x82'/*é*/, 0, 0, 0, '\x97'/*ú*/, '\xA1'/*í*/, '\xA2'/*ó*/, 0, 0, 0, 0, 0,
    '\xA0'/*á*/, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // ... rest are 0
};

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile uint8_t keyboard_read;
static volatile uint8_t keyboard_write;
static bool shift_down;
static bool caps_lock;
static bool extended_scancode;

void keyboard_initialize(void)
{
    keyboard_read = 0;
    keyboard_write = 0;
    g_InputBufferIndex = 0;
    g_InputLineReady = false;
    g_CharBuffer = -1;
    g_CharReady = false;
    g_CurrentInputMode = INPUT_MODE_NONE;
    memset(g_InputBuffer, 0, sizeof(g_InputBuffer));
    shift_down = false;
    caps_lock = false;
    extended_scancode = false;
    g_AltGrPressed = false;
    pic_initialize(0x20, 0x28);
    pic_unmask_irq(1);
}

void keyboard_irq_handler(void)
{
    uint8_t scancode;
    uint8_t next_write;
    char character;

    if ((inb(KEYBOARD_STATUS) & 1) == 0) {
        pic_send_eoi(1);
        return;
    }

    scancode = inb(KEYBOARD_DATA);
    if (scancode == 0xE0) {
        extended_scancode = true;
    } else if (extended_scancode && scancode == 0x38) {
        g_AltGrPressed = true;
        extended_scancode = false;
    } else if (extended_scancode && scancode == 0xB8) {
        g_AltGrPressed = false;
        extended_scancode = false;
    } else if (extended_scancode) {
        extended_scancode = false;
    } else if (scancode == 0x2A || scancode == 0x36) {
        shift_down = true;
    } else if (scancode == 0xAA || scancode == 0xB6) {
        shift_down = false;
    } else if (scancode == 0x3A) {
        caps_lock = !caps_lock;
    } else if ((scancode & 0x80) == 0) {
        if (g_AltGrPressed) {
            character = scancode_ascii_altgr[scancode];
        } else if (shift_down) {
            character = scancode_ascii_shifted[scancode];
        } else {
            character = scancode_ascii[scancode];
        }
        if (character >= 'a' && character <= 'z' && (shift_down != caps_lock)) {
            character = (char)(character - 'a' + 'A');
        }
        next_write = (uint8_t)(keyboard_write + 1) % KEYBOARD_BUFFER_SIZE;
        if (character != 0 && next_write != keyboard_read) {
            keyboard_buffer[keyboard_write] = character;
            keyboard_write = next_write;
        }

        if (character != 0 && g_CurrentInputMode == INPUT_MODE_GETCH) {
            g_CharBuffer = (unsigned char)character;
            g_CharReady = true;
        } else if (character != 0 && g_CurrentInputMode == INPUT_MODE_GETS && !g_InputLineReady) {
            if (character == '\n') {
                g_InputBuffer[g_InputBufferIndex] = '\0';
                g_InputLineReady = true;
                kputc('\n');
            } else if (character == '\b') {
                if (g_InputBufferIndex > 0) {
                    g_InputBufferIndex--;
                    g_InputBuffer[g_InputBufferIndex] = '\0';
                    kputc('\b');
                }
            } else if ((character >= ' ' || character == '\t') &&
                       g_InputBufferIndex < INPUT_BUFFER_SIZE - 1) {
                g_InputBuffer[g_InputBufferIndex++] = character;
                g_InputBuffer[g_InputBufferIndex] = '\0';
                kputc(character);
            }
        }
    }

    pic_send_eoi(1);
}

int keyboard_getchar(void)
{
    char character;
    if (keyboard_read == keyboard_write) {
        return -1;
    }
    character = keyboard_buffer[keyboard_read];
    keyboard_read = (uint8_t)(keyboard_read + 1) % KEYBOARD_BUFFER_SIZE;
    return (unsigned char)character;
}

// this can only run in ring0, do a linus and make a syscode for other ring abstractions to use htis instead of their keyboard
// TODO: Fix dis ASAP
void kgets(char* buffer, int32_t size) {
    if (buffer == NULL || size <= 0) {
        return;
    }

    g_CurrentInputMode = INPUT_MODE_GETS;

    for (;;) {
        __asm__ volatile("cli" ::: "memory");
        if (g_InputLineReady) {
            break;
        }
        // STI delays recognition until after HLT, avoiding a lost wakeup.
        __asm__ volatile("sti; hlt" ::: "memory");
    }

    int32_t i;
    for (i = 0; g_InputBuffer[i] != '\0' && i < size - 1; i++) {
        buffer[i] = g_InputBuffer[i];
    }
    buffer[i] = '\0';

    g_InputBufferIndex = 0;
    memset(g_InputBuffer, 0, sizeof(g_InputBuffer)); // CRITICAL: Clear buffer for next use
    g_InputLineReady = false;

    g_CurrentInputMode = INPUT_MODE_NONE;

    __asm__ volatile("sti" ::: "memory"); // Re-enable interrupts
}

int kgetch() {
    g_CurrentInputMode = INPUT_MODE_GETCH;

    for (;;) {
        __asm__ volatile("cli" ::: "memory");
        if (g_CharReady) {
            break;
        }
        // STI delays recognition until after HLT, avoiding a lost wakeup.
        __asm__ volatile("sti; hlt" ::: "memory");
    }

    int c = g_CharBuffer;
    g_CharReady = false;
    g_CharBuffer = -1;

    g_CurrentInputMode = INPUT_MODE_NONE;

    __asm__ volatile("sti" ::: "memory"); // Re-enable interrupts

    return c;
}