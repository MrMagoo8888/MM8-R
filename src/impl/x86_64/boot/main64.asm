global long_mode_start

section .text
[bits 64]

long_mode_start:
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov dword [0xb8000], 0x2f4b2f4f

    ; Multiboot pointer was in EBX (or RBX in 64-bit). 
    ; System V AMD64 ABI passes the 1st argument in RDI.
    mov rdi, rbx 

    hlt