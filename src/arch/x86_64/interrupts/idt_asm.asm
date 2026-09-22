; extern g_IDT must match C symbol name
global load_idt
extern g_IDT
section .data
idt_pointer:
    dw 256*16 - 1
    dq g_IDT
section .text
load_idt:
    lea rax, [idt_pointer]
    lidt [rax]
    ret
