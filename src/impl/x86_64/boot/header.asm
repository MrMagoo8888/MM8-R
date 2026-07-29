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

; end tag
dw 0
dw 0
dd 8

header_end: