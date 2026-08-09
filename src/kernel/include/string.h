#include "stddef.h"

void* memcpy (void *dest, const void *src, size_t len);
int memcmp (const void *str1, const void *str2, size_t count);
void* memmove (void *dest, const void *src, size_t len);
void* memset (void *dest, int val, size_t len);

char* strncpy(char *s1, const char *s2, size_t n);
char* strcpy(char *dest, const char *src);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

char* strrchr(const char *s, int c);
char* strchr(const char *s, int c);

size_t strlen(const char *s);