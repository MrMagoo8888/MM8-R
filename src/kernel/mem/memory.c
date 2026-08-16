#include "memory.h"
#include "liballoc.h"

// Ignore my yapping comments
;// it helps me understand ts

extern uint8_t __end;       // linker def for eng of kernel

static uintptr_t g_next_page = 0;       // track next free page startz var 
static uintptr_t g_page_limit = 0;      // its in da name



static uintptr_t align_up(uintptr_t value, size_t alignment) {      // aligns (rounds) adress up

    if (alignment == 0) return value;       // It dont need round
    uintptr_t mask = alignment - 1;         // make a bitmask (one less lan alignment to flip sum bits)
    return (value + mask) & ~mask;  // adds mask to value and then clears lower bits with bitwise and with inverted (~) mask to force align
}

void memory_init(uint64_t multiboot_addr) {     // winit the mem manager

    (void)multiboot_addr;       // get rid of unused warn

    uintptr_t kernel_end = align_up((uintptr_t)&__end, PAGE_SIZE);      // takes the end of kernel and rounds up to nearest page boundry
    g_next_page = kernel_end;       // sets the starting alloc pool at first safly aligned byte after kernel
    g_page_limit = kernel_end + (uintptr_t)(1024u * PAGE_SIZE);     // limits mem allocor pool to 1024 pages byond EoK (end of kernel)
}

void* memory_alloc_pages(size_t pages) {    // alloc cont. block of mem in pages

    if (g_next_page == 0) { // make sure it is inited

        memory_init(0);
    }

    if (pages == 0) {       // ye cant allocate 0 pages dumbah

        return NULL;        // go back to skewl
    }


    uintptr_t start = g_next_page;      // records startin addr of free space to ret to caller
    uintptr_t end = start + (uintptr_t)(pages * PAGE_SIZE); // calcs what new g_next_pages will be after allocin'


    if (end > g_page_limit) {   //make sure not out of mem
        return NULL;            // buy more ram newb
    }

    g_next_page = end;  // make sure next alloc happens after dis block of mem
    return (void*)start;    // resturns addr of alloced block cast to generic pointer
}

void memory_free_pages(void* ptr, size_t pages) {   // figurehead for freeing mem (liballoc job)

    // avoid warnings
    (void)ptr;  
    (void)pages;
}


// liballoc hooks ----

int liballoc_lock(void) {

    return 0;   // no multi-thread yet
}


int liballoc_unlock(void) {

    return 0;   // still no multi-threadin'
}


void* liballoc_alloc(int pages) {

    return memory_alloc_pages((size_t)pages);   // allocs pages wow
}


int liballoc_free(void* ptr, int pages) {

    memory_free_pages(ptr, (size_t)pages);  // meant to free
    return 0;


}

uint64_t allocate_physical_frame(void) {
    // request one page from the aloccer
    void* ptr = memory_alloc_pages(1);
    
    if (ptr == NULL) {
        // panic! kerel ran out of inital boot mem
        // TODO: panic routine
        //tmp:
        while(1) { __asm__ volatile("cli; hlt"); }
    }
    
    // ret raw phys addr cast to 64bit int
    return (uint64_t)ptr;
}
