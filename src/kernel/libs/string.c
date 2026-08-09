#include "stddef.h"
#include "stdint.h" 

void* memset(void *dest, int val, size_t len) {
    
    unsigned char *ptr = dest;
    unsigned char c = (unsigned char)val;

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
    while (len--) {
        *ptr++ = c;
    }
    return dest;
}

void* memcpy(void *dest, const void *src, size_t len) {

    unsigned char *d = dest;
    const unsigned char *s = src;

    // cpy 8 bytes ata time
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

    // cpy trailing bytes
    while (len--) {
        *d++ = *s++;
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

char* strncpy(char *s1, const char *s2, size_t n) {

    size_t size = internal_strnlen(s2, n);
    memcpy(s1, s2, size);
    if (size < n) {
        memset(s1 + size, '\0', n - size);
    }
    return s1;
}
