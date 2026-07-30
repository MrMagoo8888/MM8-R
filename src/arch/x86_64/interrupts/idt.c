#include "idt.h"
#include "stddef.h"

typedef struct
{
    uint16_t BaseLow;           // bits 0 - 15 of handler address
    uint16_t SegmentSelector;   // GDT code segment selector (usually 0x08)
    uint8_t  IST;               // interrupt Stack Table offset (0 for default stack)
    uint8_t  Flags;             // attribute type flags
    uint16_t BaseMid;           // bits 16 - 31 of handler address
    uint32_t BaseHigh;          // bits 32 - 63 of handler address
    uint32_t Reserved;          // Must be 0
} __attribute__((packed)) IDTEntry;

typedef struct
{
    uint16_t   Limit;
    uint64_t   Ptr;             
} __attribute__((packed)) IDTDescriptor;

// 256 vector array table entries matching the procesor spec
static IDTEntry g_IDT[256];
static IDTDescriptor g_IDTDescriptor;

extern void x86_64_IDT_Load(IDTDescriptor* idtDescriptor);

void x86_64_IDT_SetGate(int interrupt, void* base, uint16_t segmentDescriptor, uint8_t flags)
{
    uintptr_t addr = (uintptr_t)base; 

    g_IDT[interrupt].BaseLow         = (uint16_t)(addr & 0xFFFF);
    g_IDT[interrupt].SegmentSelector = segmentDescriptor;
    g_IDT[interrupt].IST             = 0;
    g_IDT[interrupt].Flags           = flags;
    g_IDT[interrupt].BaseMid         = (uint16_t)((addr >> 16) & 0xFFFF);
    g_IDT[interrupt].BaseHigh        = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    g_IDT[interrupt].Reserved        = 0;
}

void x86_64_IDT_EnableGate(int interrupt)
{
    g_IDT[interrupt].Flags |= IDT_FLAG_PRESENT;
}

void x86_64_IDT_DisableGate(int interrupt)
{
    g_IDT[interrupt].Flags &= ~IDT_FLAG_PRESENT;
}

void x86_64_IDT_Initialize()
{
    g_IDTDescriptor.Limit = sizeof(g_IDT) - 1;
    g_IDTDescriptor.Ptr   = (uint64_t)&g_IDT;

    // direct inline fallback clearing out old garbage states
    for (int i = 0; i < 256; i++) {
        x86_64_IDT_SetGate(i, NULL, 0, 0);
    }

    // call 64-bit assembly loader
    x86_64_IDT_Load(&g_IDTDescriptor);
}
