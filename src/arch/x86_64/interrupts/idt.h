#ifndef IDT_H
#define IDT_H

#include "stdint.h"


// to calculate you find b values:
/*
Ring0: 0b00
Ring1: 0b01
Ring2: 0b10
Ring3: 0b11
*/

// Then, shift the ring num left by 5 to move into bits 5 and 6
// This then makes it:
/*
Ring0: 0b000000
Ring1: 0b010000
Ring2: 0b100000
Ring3: 0b110000
*/

// Then make hex:
/*
Ring0: 0x00
Ring1: 0x20
Ring2: 0x40
Ring3: 0x60
*/
typedef enum {
    IDT_FLAG_GATE_64BIT_INT   = 0x0E, // 64-bit Interrupt Gate type
    IDT_FLAG_GATE_64BIT_TRAP  = 0x0F, // 64-bit Trap Gate type
    
    IDT_FLAG_RING0            = 0x00, // Kernel privilege
    IDT_FLAG_RING1            = 0x20,
    IDT_FLAG_RING2            = 0x40,
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
