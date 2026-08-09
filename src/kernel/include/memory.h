#ifndef MEMORY_H
#define MEMORY_H

#include "stdint.h"
#include "stddef.h"

#define PAGE_SIZE 4096u // u = unsigned

void memory_init(uint64_t multiboot_addr);
void* memory_alloc_pages(size_t pages);
void memory_free_pages(void* ptr, size_t pages);

#endif
