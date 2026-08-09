section .data
    align 4

; define 10-byte IDTR structure (2-byte limit + 8-byte base address)
idt_pointer:
    dw 256 * 16 - 1      ; limit - size of IDT in bytes minus 1 (256 entries * 16 bytes each)
    dq idt_table         ; base - 64-bit absolute address pointing to IDT array

section .bss
    align 16
; reserve uninitialized space for IDT table (256 entries * 16 bytes = 4096 bytes)
idt_table:
    resb 4096

section .text
global load_idt


load_idt:
    lidt [idt_pointer]
    ret 
