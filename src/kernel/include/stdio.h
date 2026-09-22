#pragma once
#include "stdint.h"
#include "stdbool.h"
//#include "arch/i686/screen_defs.h"

#define DEFAULT_COLOR 0x07  // light gray text on black
#define SCROLLBACK_LINES 10

extern bool g_ConsoleAutoSwap;
extern uint32_t g_ConsoleDelay;

void console_initialize();
void console_set_font_scale(int scale);
void clrscr();
void kputc(char c);
void kputs(const char* str);
void kprintf(const char* fmt, ...);
void kprint_buffer(const char* msg, const void* buffer, uint32_t count);
char kgetchr(int x, int y);
void kputcolor(int x, int y, uint8_t color);
void kputchr(int x, int y, char c);
uint8_t kgetcolor(int x, int y);
void setcursor(int x, int y);
void scrollback(int lines);
void scrollforward(int lines);
void console_refresh();
void refresh_screen_color();

void view_scrollback_up();
void view_scrollback_down();

int sprintf(char* str, const char* format, ...);
int sscanf(const char* str, const char* format, ...);