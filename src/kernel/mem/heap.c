#include "heap.h"
#include "memory.h"

// ported old heap just in case
// modeled it out of file so i aint recommenting it, i want it clean

typedef struct block_header {
    size_t size;
    bool is_free;
    struct block_header* next;
} block_header_t;

static block_header_t* heap_start = NULL;

static void* heap_memcpy(void* dst, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i = 0; i < n; ++i) {
        d[i] = s[i];
    }
    return dst;
}

void heap_initialize(void) {
    if (heap_start != NULL) {
        return;
    }

    void* base = memory_alloc_pages(1);
    if (base == NULL) {
        return;
    }

    heap_start = (block_header_t*)base;
    heap_start->size = PAGE_SIZE - sizeof(block_header_t);
    heap_start->is_free = true;
    heap_start->next = NULL;
}

void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    if (heap_start == NULL) {
        heap_initialize();
    }

    if (heap_start == NULL) {
        return NULL;
    }

    size_t aligned_size = (size + 15u) & ~15u;
    size_t needed = aligned_size + sizeof(block_header_t);

    block_header_t* current = heap_start;
    while (current != NULL) {
        if (current->is_free && current->size >= needed) {
            size_t leftover = current->size - needed;
            if (leftover > sizeof(block_header_t) + 16u) {
                block_header_t* new_block = (block_header_t*)((char*)current + needed);
                new_block->size = leftover;
                new_block->is_free = true;
                new_block->next = current->next;

                current->size = aligned_size;
                current->next = new_block;
            }

            current->is_free = false;
            return (void*)((char*)current + sizeof(block_header_t));
        }

        current = current->next;
    }

    return NULL;
}

void* kmalloc_aligned(size_t size, size_t alignment) {
    if (size == 0) {
        return NULL;
    }
    if (alignment == 0) {
        return kmalloc(size);
    }
    if ((alignment & (alignment - 1)) != 0) {
        return NULL;
    }

    size_t total_size = size + alignment + sizeof(void*);
    void* raw_ptr = kmalloc(total_size);
    if (raw_ptr == NULL) {
        return NULL;
    }

    uintptr_t raw_addr = (uintptr_t)raw_ptr;
    uintptr_t aligned_addr = (raw_addr + sizeof(void*) + (alignment - 1)) & ~(alignment - 1);

    ((void**)aligned_addr)[-1] = raw_ptr;
    return (void*)aligned_addr;
}

void kfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    block_header_t* header = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    header->is_free = true;

    block_header_t* current = heap_start;
    while (current != NULL && current->next != NULL) {
        if (current->is_free && current->next->is_free) {
            current->size += sizeof(block_header_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

void kfree_aligned(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    void* raw_ptr = ((void**)ptr)[-1];
    kfree(raw_ptr);
}

void* krealloc(void* ptr, size_t new_size) {
    if (ptr == NULL) {
        return kmalloc(new_size);
    }

    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    block_header_t* header = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    if (new_size <= header->size) {
        return ptr;
    }

    void* new_ptr = kmalloc(new_size);
    if (new_ptr == NULL) {
        return NULL;
    }

    heap_memcpy(new_ptr, ptr, header->size);
    kfree(ptr);
    return new_ptr;
}

void heap_get_stats(size_t* total, size_t* used, size_t* free_mem) {
    if (total != NULL) {
        *total = 0;
    }
    if (used != NULL) {
        *used = 0;
    }
    if (free_mem != NULL) {
        *free_mem = 0;
    }

    if (heap_start == NULL) {
        return;
    }

    block_header_t* current = heap_start;
    while (current != NULL) {
        if (current->is_free) {
            if (free_mem != NULL) {
                *free_mem += current->size;
            }
        } else {
            if (used != NULL) {
                *used += current->size;
            }
        }
        current = current->next;
    }

    if (total != NULL) {
        *total = *used + *free_mem;
    }
}
