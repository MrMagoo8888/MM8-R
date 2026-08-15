section .multiboot_header
align 8
header_start:
    ; Magic number
    dd 0xe85250d6
    ; Architecture (0 = Protected Mode i386)
    dd 0 
    ; Header length
    dd header_end - header_start
    ; Checksum 
    dd -(0xe85250d6 + 0 + (header_end - header_start))

    ; info (copied from docs)
    align 8
    dw 1                        ; Type 1: Information request
    dw 0                        ; Flags (0 = required)
    dd 20                       ; Tag size: 8 (header) + 4 (FB) + 4 (ACPI1) + 4 (ACPI2) = 20 bytes
    dd 12                       ; Request Tag Type 12: Framebuffer / Graphic info
    dd 14                       ; Request Tag Type 14: ACPI 1.0 RSDP
    dd 15                       ; Request Tag Type 15: ACPI 2.0+ XSDT RSDP

    ; framebuffa (for vbe)
    align 8
    dw 5                        ; Tag type 5 (Framebuffer request)
    dw 0                        ; Flags
    dd 20                       ; Tag size (20 bytes)
    dd 1920                     ; Preferred Width
    dd 1080                     ; Preferred Height
    dd 32                       ; Preferred Bits Per Pixel

    ; end tag
    align 8
    dw 0                        ; Type 0
    dw 0                        ; Flags
    dd 8                        ; Size

header_end: