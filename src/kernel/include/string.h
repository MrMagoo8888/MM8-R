#ifndef STRING_H
#define STRING_H

#include "stddef.h"

// mem funcs
void* memcpy(void* dst, const void* src, size_t num);
void* memset(void* ptr, int value, size_t num);
void* memmove(void* dst, const void* src, size_t num);
int memcmp(const void* ptr1, const void* ptr2, size_t num);

// string funcs
int strcmp(const char* str1, const char* str2);
size_t strlen(const char* str);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, size_t n);
int strncmp(const char* str1, const char* str2, size_t n);
const char* strchr(const char* str, int c);
const char* strrchr(const char* str, int c);

#endif
