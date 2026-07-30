#include "heap.h"
#include "string.h"  // includes mem
#include "stdbool.h"
#include "stdint.h"
#include "stddef.h"  

typedef struct block_header {
    size_t size;
    struct block_header* next;
    bool is_free;
    uint8_t padding[7]; // Explicit padding to force total struct size = 24 bytes + 8 bytes = 32 bytes
} __attribute__((aligned(16))) block_header_t;

// the linker will provide..
extern uint8_t __end;

// start of heap (mem blocks)
static block_header_t* heap_start = NULL;

// 512mb heap (8gb rn tots)
#define HEAP_SIZE (1024 * 1024 * 512)

void heap_initialize() {
    uintptr_t heap_addr = (uintptr_t)&__end;
    
    // initial address perfectly aligned to a 16-byte boundary?
    if (heap_addr % 16 != 0) {
        heap_addr += 16 - (heap_addr % 16);
    }
    heap_start = (block_header_t*)heap_addr;

    // initialy there is one large free block
    heap_start->size = HEAP_SIZE - sizeof(block_header_t);
    heap_start->is_free = true;
    heap_start->next = NULL;
}

void* malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    // force requested alloc sizes to be multiples of 16 bytes
    if (size % 16 != 0) {
        size += 16 - (size % 16);
    }

    block_header_t* current = heap_start;
    while (current) {
        if (current->is_free && current->size >= size) {
            // is block large enough to split?
            if (current->size > size + sizeof(block_header_t)) {
                // create a new header for the remaining bit of block
                block_header_t* new_block = (block_header_t*)((uint8_t*)current + sizeof(block_header_t) + size);
                new_block->size = current->size - size - sizeof(block_header_t);
                new_block->is_free = true;
                new_block->next = current->next;

                // update current block
                current->size = size;
                current->next = new_block;
            }

            current->is_free = false;
            // return pointer to memory region after header
            return (void*)((uint8_t*)current + sizeof(block_header_t));
        }
        current = current->next;
    }

    return NULL;
}

void* malloc_aligned(size_t size, size_t alignment) {
    if (size == 0) return NULL;
    if (alignment == 0) return malloc(size);

    // Ensure alignment is a power of two
    if (alignment & (alignment - 1)) return NULL;

    // tot size: requested + alignment padding + space for pointer storage
    size_t total_size = size + alignment + sizeof(void*);
    void* raw_ptr = malloc(total_size);
    if (!raw_ptr) return NULL;

    uintptr_t raw_addr = (uintptr_t)raw_ptr;
    uintptr_t aligned_addr = (raw_addr + sizeof(void*) + (alignment - 1)) & ~(alignment - 1);

    // store original raw pointer immediately before aligned address.
    ((void**)aligned_addr)[-1] = raw_ptr;

    return (void*)aligned_addr;
}

void free_aligned(void* ptr) {
    if (!ptr) return;
    void* raw_ptr = ((void**)ptr)[-1];
    free(raw_ptr);
}

void free(void* ptr) {
    if (!ptr) {
        return;
    }

    block_header_t* header = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
    header->is_free = true;

    // Coalesce (big girl word) adjacent free blocks to prevent fragmentation (another one)
    block_header_t* current = heap_start;
    while (current && current->next) {
        if (current->is_free && current->next->is_free) {
            current->size += sizeof(block_header_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

void* realloc(void* ptr, size_t new_size) {
    if (!ptr) {
        return malloc(new_size);
    }

    if (new_size == 0) {
        free(ptr);
        return NULL;
    }

    block_header_t* header = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
    
    if (new_size <= header->size) {
        return ptr;
    }

    void* new_ptr = malloc(new_size);
    if (!new_ptr) {
        return NULL;
    }
    memcpy(new_ptr, ptr, header->size);
    free(ptr);
    return new_ptr;
}

void heap_get_stats(size_t* total, size_t* used, size_t* free_mem) {
    *total = HEAP_SIZE;
    *used = 0;
    *free_mem = 0;

    block_header_t* current = heap_start;
    while (current) {
        if (current->is_free) {
            *free_mem += current->size;
        } else {
            *used += current->size;
        }
        current = current->next;
    }
}
