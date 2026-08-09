#include "idt.h"
#include "stdint.h"
#include "binary.h"




typedef struct
{
    uint16_t BaseLow;           // Offset bits 0..15
    uint16_t SegmentSelector;   // GDT Code Segment selector
    uint8_t  IST;               // Interrupt Stack Table offset (bits 0..2), rest is 0
    uint8_t  Flags;             // Type and attributes
    uint16_t BaseMid;           // Offset bits 16..31
    uint32_t BaseHigh;          // Offset bits 32..63
    uint32_t Reserved;          // Reserved, must be 0
} __attribute__((packed)) IDTEntry;


typedef struct
{
    uint16_t   Limit;
    IDTEntry*  Ptr;             
} __attribute__((packed)) IDTDescriptor;


IDTEntry g_IDT[256];


IDTDescriptor g_IDTDescriptor = { sizeof(g_IDT) - 1, g_IDT };


extern void load_idt(void);



void x86_64_IDT_SetGate(int interrupt, void* base, uint16_t segmentDescriptor, uint8_t flags)
{
    uint64_t address = (uint64_t)base;

    g_IDT[interrupt].BaseLow         = address & 0xFFFF;
    g_IDT[interrupt].SegmentSelector = segmentDescriptor;
    g_IDT[interrupt].IST             = 0; // Default to 0 unless you configure an IST in your TSS
    g_IDT[interrupt].Flags           = flags;
    g_IDT[interrupt].BaseMid         = (address >> 16) & 0xFFFF;
    g_IDT[interrupt].BaseHigh        = (address >> 32) & 0xFFFFFFFF;
    g_IDT[interrupt].Reserved        = 0;
}

void x86_64_IDT_EnableGate(int interrupt)
{
    FLAG_SET(g_IDT[interrupt].Flags, IDT_FLAG_PRESENT);
}


void x86_64_IDT_DisableGate(int interrupt)
{
    FLAG_UNSET(g_IDT[interrupt].Flags, IDT_FLAG_PRESENT);
}



void x86_64_IDT_Initialize()
{

    x86_64_ISR_InitializeGates();


    load_idt();
}
