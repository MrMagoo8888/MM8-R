[bits 64]

global x86_64_IDT_Load
x86_64_IDT_Load:
    ; RDI auto-y holds address of IDTDescriptor structure
    lidt [rdi]
    ret
