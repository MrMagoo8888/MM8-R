#ifndef GDT_H
#define GDT_H

#include "stdint.h"


#define x86_64_GDT_CODE_SEGMENT 0x08 // offset to Kernel Code Entry
#define x86_64_GDT_DATA_SEGMENT 0x10 // offset to Kernel Data Entry
#define x86_64_GDT_TSS_SEGMENT  0x28 // offset to Wide TSS Entry (5th 8-byte slot)


void x86_64_GDT_Initialize(void);
void x86_64_TSS_SetStack(uint64_t kernelRSP);


// sets up segments und handles 64-bit far jump tracking sequence
extern void x86_64_GDT_Load(void* descriptor, uint16_t codeSegment, uint16_t dataSegment);

#endif 
