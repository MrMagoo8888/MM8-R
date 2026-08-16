#include "stddef.h"
#include "stdint.h" 

void* memset(void *dest, int val, size_t len) {

    unsigned char *ptr = (unsigned char*)dest;
    unsigned char c = (unsigned char)val;

    // align dest pointer to 8-byte bound
    while(len > 0 && ((uintptr_t)ptr & 7) != 0) {
        *ptr++ = c;
        len--;
    }

    // optimize for 64-bit chunks if len is large enough
    if (len >= 8) {
        // broaden single byte to full 64-bit pattern
        uint64_t val64 = c;
        val64 |= (val64 << 8);
        val64 |= (val64 << 16);
        val64 |= (val64 << 32);

        uint64_t *ptr64 = (uint64_t*)ptr;
        while (len >= 8) {
            *ptr64++ = val64;
            len -= 8;
        }
        ptr = (unsigned char*)ptr64;
    }

    // clean remaining bytes
    while (len > 0) {
        *ptr++ = c;
        len--;
    }
    return dest;
}



void* memcpy(void *dest, const void *src, size_t len) {

    unsigned char *d = (unsigned char*)dest;
    const unsigned char *s = (unsigned char*)src;

    // check if ptrs share same relitive alignmnt
    // if lowest 3bits dont match they cant be aligned at same time
    if (((uintptr_t)d & 7) == ((uintptr_t)s & 7)) {
        while (len > 0 && ((uintptr_t)d & 7) != 0) {
            *d++ = *s++;
            len--;
        }

        // both are now 8-byt alignd do fast 64bit transfrs
        if (len >= 8) {
            uint64_t *d64 = (uint64_t*)d;
            const uint64_t *s64 = (const uint64_t*)s;
            while (len >= 8) {
                *d64++ = *s64++;
                len -= 8;
            }
            d = (unsigned char*)d64;
            s = (const unsigned char*)s64;

        }

    }

    // cpy trailing bytes
    while (len > 0) {
        *d++ = *s++;
        len--;
    }
    return dest;
}

void* memmove(void *dest, const void *src, size_t len) {

    unsigned char *d = dest;
    const unsigned char *s = src;

    if (len == 0 || d == s) return dest;

    // if source and dest overlap and dest is ahead, copy backwards
    if (d > s && d < s + len) {
        d += len;
        s += len;
        
        if (len >= 8) {
            uint64_t *d64 = (uint64_t*)d;
            const uint64_t *s64 = (const uint64_t*)s;
            while (len >= 8) {
                *--d64 = *--s64;
                len -= 8;
            }
            d = (unsigned char*)d64;
            s = (const unsigned char*)s64;
        }
        while (len--) {
            *--d = *--s;
        }
    } else {
        // no overlap, safe forward copy
        return memcpy(dest, src, len);
    }
    return dest;
}

int memcmp(const void *str1, const void *str2, size_t count) {

    const unsigned char *s1 = str1;
    const unsigned char *s2 = str2;

    while (count--) {
        if (*s1 != *s2) {
            return (*s1 < *s2) ? -1 : 1;
        }
        s1++;
        s2++;
    }
    return 0;
}

static size_t internal_strnlen(const char *s, size_t maxlen) {

    size_t len = 0;
    while (len < maxlen && s[len] != '\0') {
        len++;
    }
    return len;
}

char* strncpy(char *s1, const char *s2, size_t n) {     // might page fault

    size_t size = internal_strnlen(s2, n);
    memcpy(s1, s2, size);
    if (size < n) {
        memset(s1 + size, '\0', n - size);
    }
    return s1;
}

char* strcpy(char *dest, const char *src) {  // might page fault
    size_t len = 0;
    
    // Find the length of the string
    while (src[len] != '\0') {
        len++;
    }
    
    // cpy the string data + 1 byte for the '\0' terminator
    memcpy(dest, src, len + 1);
    
    return dest;
}

int strcmp(const char *s1, const char *s2) {         // might page fault
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    while (*p1 && (*p1 == *p2)) {
        p1++;
        p2++;
    }

    // return negative value if s1 < s2, 0 if equal, positive if s1 > s2
    return *p1 - *p2;
}


int strncmp(const char *s1, const char *s2, size_t n) {
    // if n is 0, the strings are equal by def
    if (n == 0) return 0;

    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    // decrement n each iterationstop if characters mismatch or \0 
    while (n > 1 && *p1 && (*p1 == *p2)) {
        p1++;
        p2++;
        n--;
    }

    // eval final character difference
    return *p1 - *p2;
}


char* strchr(const char *s, int c) {
    char target = (char)c;

    // scan until found character or hit null term
    while (*s != target) {
        if (*s == '\0') {
            return NULL; // char not found
        }
        s++;
    }

    // return the pointer casting away constness
    return (char *)s;
}



char* strrchr(const char *s, int c) {
    char target = (char)c;
    size_t len = 0;

    // find end of string
    while (s[len] != '\0') {
        len++;
    }

    size_t i = len + 1; // include null terminator in search space
    while (i > 0) {
        i--;
        if (s[i] == target) {
            return (char *)(s + i); // found rightmost match
        }
    }

    return NULL; // cahr not found
}



size_t strlen(const char *s) {
    const char *p = s;
    while (*p != '\0') {
        p++;
    }
    return (size_t)(p - s);
}

