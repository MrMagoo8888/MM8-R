#include "keyboard.h"
#include "io.h"
#include "pic.h"
#include "stdbool.h"

#define KEYBOARD_DATA 0x60
#define KEYBOARD_STATUS 0x64
#define KEYBOARD_BUFFER_SIZE 64

static const char keymap[128] = {
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
};

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile uint8_t keyboard_read;
static volatile uint8_t keyboard_write;
static bool shift_down;
static bool caps_lock;

void keyboard_initialize(void)
{
    keyboard_read = 0;
    keyboard_write = 0;
    shift_down = false;
    caps_lock = false;
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
    if (scancode == 0x2A || scancode == 0x36) {
        shift_down = true;
    } else if (scancode == 0xAA || scancode == 0xB6) {
        shift_down = false;
    } else if (scancode == 0x3A) {
        caps_lock = !caps_lock;
    } else if ((scancode & 0x80) == 0) {
        character = keymap[scancode];
        if (character >= 'a' && character <= 'z' && (shift_down != caps_lock)) {
            character = (char)(character - 'a' + 'A');
        }
        next_write = (uint8_t)(keyboard_write + 1) % KEYBOARD_BUFFER_SIZE;
        if (character != 0 && next_write != keyboard_read) {
            keyboard_buffer[keyboard_write] = character;
            keyboard_write = next_write;
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