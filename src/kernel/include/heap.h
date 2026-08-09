#ifndef HEAP_H  // idk why im trying but oh well
#define HEAP_H

#include "stddef.h"
#include "stdint.h"
#include "stdbool.h"

void heap_initialize(void);
void* kmalloc(size_t size);
void* kmalloc_aligned(size_t size, size_t alignment);
void kfree(void* ptr);
void kfree_aligned(void* ptr);
void* krealloc(void* ptr, size_t new_size);
void heap_get_stats(size_t* total, size_t* used, size_t* free_mem);

#endif