#include "string.h"
#include "stddef.h"
#include "stdint.h" 

int strcmp(const char* str1, const char* str2) {    // accepts pointer inputs without modifying original strings
    while (*str1 && (*str1 == *str2)) {     // checks if current char of str1 is not null terminator (\0), checks if both strings match, if y then both conditions
        str1++;     // moves to next charecter
        str2++;     // same
    }
    return *(const unsigned char*)str1 - *(const unsigned char*)str2;   // casts pinters to unsigned chars, ensures extended ascii are treated positive, subtracts charecter values where the loop stopped
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

char* strcpy(char* dst, const char* src) {
    char* origDst = dst;
    while (*src) {
        *dst++ = *src++;
    }
    *dst = '\0';
    return origDst;
}

char* strncpy(char* dst, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    for (; i < n; i++) {
        dst[i] = '\0';
    }
    return dst;
}

int strncmp(const char* str1, const char* str2, size_t n) {
    for(size_t i = 0; i < n; i++) {
        if (str1[i] != str2[i]) {
            return (unsigned char)str1[i] - (unsigned char)str2[i];
        }
        if (str1[i] == '\0') {
            return 0;   // both strings are equal up to n or less
        }
    }
    return 0;   // strins are wqual for n chars
}

const char* strchr(const char* str, int c) {
    while (*str != (const)c) {
        if (!*str++) {
            return 0;
        }
    }
    return str;
}

const char* strrchr(const char* str, int c) {
    const char* last = 0;
    do {
        if (*str == (char)c) {
            last = str;
        }
    } while (*str++);
    return last;
}

void* memcpy(void* dst, const void* src, size_t num)
{
    // 64-bit Optimization: Copy 8 bytes (uint64_t) at a time if aligned
    if (((uintptr_t)dst % 8 == 0) && ((uintptr_t)src % 8 == 0) && (num % 8 == 0)) {
        uint64_t* u64Dst = (uint64_t *)dst;
        const uint64_t* u64Src = (const uint64_t *)src;
        size_t n = num / 8;
        for (size_t i = 0; i < n; i++)
            u64Dst[i] = u64Src[i];
        return dst;
    }

    // Fallback forward copy byte-by-byte
    uint8_t* u8Dst = (uint8_t *)dst;
    const uint8_t* u8Src = (const uint8_t *)src;

    for (size_t i = 0; i < num; i++)
        u8Dst[i] = u8Src[i];

    return dst;
}

void* memset(void* ptr, int value, size_t num)
{
    uint8_t* u8Ptr = (uint8_t*)ptr;

    // 64-bit Optimization: Fill 8 bytes (uint64_t) at a time if possible
    if (num >= 8 && ((uintptr_t)ptr % 8 == 0)) {
        uint64_t v64 = (uint8_t)value;
        v64 |= (v64 << 8);
        v64 |= (v64 << 16);
        v64 |= (v64 << 24);
        v64 |= (v64 << 32);
        v64 |= (v64 << 40);
        v64 |= (v64 << 48);
        v64 |= (v64 << 56);

        uint64_t* u64Ptr = (uint64_t*)ptr;
        size_t n64 = num / 8;
        for (size_t i = 0; i < n64; i++) {
            u64Ptr[i] = v64;
        }

        u8Ptr += n64 * 8;
        num %= 8;
    }

    while (num--) {
        *u8Ptr++ = (uint8_t)value;
    }

    return ptr;
}

int memcmp(const void* ptr1, const void* ptr2, size_t num)
{
    const uint8_t* u8Ptr1 = (const uint8_t *)ptr1;
    const uint8_t* u8Ptr2 = (const uint8_t *)ptr2;

    for (size_t i = 0; i < num; i++)
    {
        if (u8Ptr1[i] != u8Ptr2[i])
            return (int)u8Ptr1[i] - (int)u8Ptr2[i];
    }

    return 0;
}

void* memmove(void* dst, const void* src, size_t num)
{
    uint8_t* u8Dst = (uint8_t*)dst;
    const uint8_t* u8Src = (const uint8_t*)src;

    // Direct pointer comparison is perfectly legal and safe in 64-bit flat mode
    if (u8Dst < u8Src) {
        for (size_t i = 0; i < num; i++) {
            u8Dst[i] = u8Src[i];
        }
    } else { 
        for (size_t i = num; i > 0; i--) {
            u8Dst[i-1] = u8Src[i-1];
        }
    }

    return dst;
}