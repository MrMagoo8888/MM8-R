section .multiboot_header
header_start:
;Magik num
dd 0xe85250d6
; Arcitecture
dd 0 ; Prot mode
; Header length
dd header_end - header_start
;checksum
dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    ; Framebuffa (vbe)
    align 8
    dw 5                         ; Tag type 5 (Framebuffer request)
    dw 0                         ; Flags
    dd 20                        ; Tag size (20 bytes)
    dd 1920                     ; Preferred Width (e.g., 1024)
    dd 1080                      ; Preferred Height (e.g., 768)
    dd 32                        ; Preferred Bits Per Pixel (32-bit color)
    align 8

; end tag
dw 0
dw 0
dd 8

header_end: