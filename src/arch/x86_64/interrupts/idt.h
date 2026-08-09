#ifndef IDT_H
#define IDT_H

#include "stdint.h"


typedef enum {
    IDT_FLAG_GATE_64BIT_INT   = 0x0E, // 64-bit Interrupt Gate type
    IDT_FLAG_GATE_64BIT_TRAP  = 0x0F, // 64-bit Trap Gate type
    
    IDT_FLAG_RING0            = 0x00, // Kernel privilege
    IDT_FLAG_RING3            = 0x60, // User privilege
    
    IDT_FLAG_PRESENT          = 0x80  // Segment Present bit
} IDT_FLAGS;


void x86_64_IDT_Initialize(void);
void x86_64_IDT_SetGate(int interrupt, void* base, uint16_t segmentDescriptor, uint8_t flags);
void x86_64_IDT_EnableGate(int interrupt);
void x86_64_IDT_DisableGate(int interrupt);


void x86_64_ISR_InitializeGates(void);


extern void load_idt(void);

#endif 
