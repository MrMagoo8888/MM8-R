global long_mode_start
extern kernel_main
extern stack_top

section .text
[bits 64]

long_mode_start:
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; The multiboot2 information pointer is passed in EBX from the bootloader
    ; Preserve it across the long-mode transition and pass it to the C kernel as
    ; the first argument using the System V AMD64 ABI.
    mov rdi, rbx

    ; set up a valid 64-bit stack before entering C code
    lea rsp, [rel stack_top]
    and rsp, ~15

    call kernel_main

    cli
    hlt