#include "gdt.h"
#include "stdint.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"


typedef struct
{
    uint16_t LimitLow;                  
    uint16_t BaseLow;                   
    uint8_t  BaseMiddle;                 
    uint8_t  Access;                     
    uint8_t  FlagsLimitHi;               
    uint8_t  BaseHigh;                   
} __attribute__((packed)) GDTEntry;


typedef struct
{
    GDTEntry StandardGate;              // The traditional lower 8 bytes
    uint32_t BaseUpper;                 // Bits 32-63 of the 64-bit address
    uint32_t Reserved;                  // Must be 0
} __attribute__((packed)) TSSEntryGDT;

typedef struct
{
    uint16_t Limit;                     // Sizeof GDT minus 1 (10 bytes total structure)
    void*    Ptr;                       // 64-bit pointer to the GDT array
} __attribute__((packed)) GDTDescriptor;


typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;                      // Ring 0 Stack Pointer (Replaces esp0)
    uint64_t rsp1;                      // Ring 1 Stack Pointer
    uint64_t rsp2;                      // Ring 2 Stack Pointer
    uint64_t reserved1;
    uint64_t ist[7];                    // Interrupt Stack Tables (IST 1 through 7)
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;                // I/O Map Base Address
} __attribute__((packed)) TSSEntry;

static TSSEntry g_TSS;


void x86_64_TSS_SetStack(uint64_t kernelRSP) {
    g_TSS.rsp0 = kernelRSP;
}


typedef enum
{
    GDT_ACCESS_CODE_READABLE                = 0x02,
    GDT_ACCESS_DATA_WRITEABLE               = 0x02,
    GDT_ACCESS_DATA_SEGMENT                 = 0x10,
    GDT_ACCESS_CODE_SEGMENT                 = 0x18,
    GDT_ACCESS_DESCRIPTOR_TSS               = 0x09, // 64-bit Available TSS type is 0x9
    GDT_ACCESS_RING0                        = 0x00,
    GDT_ACCESS_RING3                        = 0x60,
    GDT_ACCESS_PRESENT                      = 0x80,
} GDT_ACCESS;

typedef enum 
{
    GDT_FLAG_64BIT                          = 0x20, // Crucial: sets the 'L' bit for Long Mode
    GDT_FLAG_32BIT                          = 0x40,
    GDT_FLAG_GRANULARITY_4K                 = 0x80,
} GDT_FLAGS;

#define GDT_LIMIT_LOW(limit)                (limit & 0xFFFF)
#define GDT_BASE_LOW(base)                  (base & 0xFFFF)
#define GDT_BASE_MIDDLE(base)               ((base >> 16) & 0xFF)
#define GDT_FLAGS_LIMIT_HI(limit, flags)    (((limit >> 16) & 0xF) | (flags & 0xF0))
#define GDT_BASE_HIGH(base)                 ((base >> 24) & 0xFF)

// Helper macro for code/data segments. In 64-bit, base and limit are natively 0.
#define GDT_ENTRY(base, limit, access, flags) {                     \
    GDT_LIMIT_LOW(limit),                                           \
    GDT_BASE_LOW(base),                                             \
    GDT_BASE_MIDDLE(base),                                          \
    access,                                                         \
    GDT_FLAGS_LIMIT_HI(limit, flags),                               \
    GDT_BASE_HIGH(base)                                             \
}


struct {
    GDTEntry Null;
    GDTEntry KernelCode;    // 0x08
    GDTEntry KernelData;    // 0x10

    GDTEntry Ring1Code;     // 0x18
    GDTEntry Ring1Data;     // 0x20

    GDTEntry Ring2Code;     // 0x28
    GDTEntry Ring2Data;     // 0x30

    GDTEntry UserCode;      // 0x38
    GDTEntry UserData;      // 0x40
    TSSEntryGDT TSS;        // 0x48
} __attribute__((packed)) g_GDT = {
    GDT_ENTRY(0, 0, 0, 0),

    // ring 0 
    GDT_ENTRY(0, 0, GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE, GDT_FLAG_64BIT),
    GDT_ENTRY(0, 0, GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE, 0),

    // ring 1
    GDT_ENTRY(0, 0, GDT_ACCESS_PRESENT | 0x20 | GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE, GDT_FLAG_64BIT),
    GDT_ENTRY(0, 0, GDT_ACCESS_PRESENT | 0x20 | GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE, 0),

    // ring2
    GDT_ENTRY(0, 0, GDT_ACCESS_PRESENT | 0x40 | GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE, GDT_FLAG_64BIT),
    GDT_ENTRY(0, 0, GDT_ACCESS_PRESENT | 0x40 | GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE, 0),

    // ring 3 
    GDT_ENTRY(0, 0, GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE, GDT_FLAG_64BIT),
    GDT_ENTRY(0, 0, GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE, 0),

    // TSS alloc
    { GDT_ENTRY(0, 0, 0, 0), 0, 0 } 
};

GDTDescriptor g_GDTDescriptor = { sizeof(g_GDT) - 1, &g_GDT };


void x86_64_TSS_SetStacks(uint64_t ring0RSP, uint64_t ring1RSP, uint64_t ring2RSP) {
    g_TSS.rsp0 = ring0RSP;
    g_TSS.rsp1 = ring1RSP;
    g_TSS.rsp2 = ring2RSP;
}

void x86_64_GDT_Initialize()
{
    memset(&g_TSS, 0, sizeof(g_TSS));
    g_TSS.iomap_base = sizeof(g_TSS);

    uint64_t tss_base = (uint64_t)&g_TSS;
    uint32_t tss_limit = sizeof(g_TSS) - 1;

    g_GDT.TSS.StandardGate.LimitLow    = GDT_LIMIT_LOW(tss_limit);
    g_GDT.TSS.StandardGate.BaseLow     = GDT_BASE_LOW(tss_base);
    g_GDT.TSS.StandardGate.BaseMiddle  = GDT_BASE_MIDDLE(tss_base);
    g_GDT.TSS.StandardGate.Access      = GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DESCRIPTOR_TSS; 
    g_GDT.TSS.StandardGate.FlagsLimitHi = GDT_FLAGS_LIMIT_HI(tss_limit, 0);
    g_GDT.TSS.StandardGate.BaseHigh    = GDT_BASE_HIGH(tss_base);
    g_GDT.TSS.BaseUpper                = (tss_base >> 32) & 0xFFFFFFFF;
    g_GDT.TSS.Reserved                 = 0;

    x86_64_GDT_Load(&g_GDTDescriptor, 0x08, 0x10);
    
    __asm__ volatile("ltr %%ax" : : "a" (0x48)); 
}

// Load GDT and update segment registers. Implemented in C with inline asm
// to avoid needing a separate assembly file.
void x86_64_GDT_Load(void* descriptor, uint16_t codeSegment, uint16_t dataSegment)
{
    // Load GDTR
    __asm__ volatile ("lgdt (%0)" : : "r" (descriptor));

    // Load data segment registers (DS, ES, FS, GS, SS)
    __asm__ volatile (
        "mov %0, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        : : "r" (dataSegment) : "rax"
    );

    // Far return to reload CS register. Push selector then RIP and lretq.
    __asm__ volatile (
        "pushq %0\n\t"
        "leaq 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"
        "lretq\n\t"
        "1:\n\t"
        : : "r" ((uint64_t)codeSegment) : "rax"
    );
}
