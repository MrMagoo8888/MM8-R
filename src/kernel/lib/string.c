#include "string.h"
#include "stddef.h"

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
            return(unsigned char)str1[i] = (unsigned char)str2[i];
        }
        if (str1[i] == '\0') {
            return 0;   // both strings are equal up to n or less
        }
    }
    return 0;   // strins are wqual for n chars
}

const char*