#ifndef HEAP_H
#define HEAP_H

#include "stddef.h"

void heap_initialize(void);
void* malloc(size_t size);
void* malloc_aligned(size_t size, size_t alignment);
void free_aligned(void* ptr);
void free(void* ptr);
void* realloc(void* ptr, size_t new_size);
void heap_get_stats(size_t* total, size_t* used, size_t* free_mem);

#endif
