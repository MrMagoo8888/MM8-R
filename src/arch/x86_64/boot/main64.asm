global long_mode_start
extern kernel_main
extern stack_top

section .text
[bits 64]

long_mode_start:
    ; Preserve the multiboot values before clearing low registers for the 64-bit
    ; mode transition. The bootloader passes the info pointer in EBX and the
    ; magic number in EAX.
    mov rdi, rbx    ; multiboot info pointer
    mov rsi, rax    ; multiboot magic value

    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; set up a valid 64-bit stack before entering C code
    lea rsp, [rel stack_top]
    and rsp, ~15

    call kernel_main

    cli
    hlt