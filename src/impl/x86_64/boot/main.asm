global header_start
global start                   ; Usually required by linkers
extern long_mode_start

section .text
[bits 32]

_start:
start:
    mov esp, stack_top

    call check_multiboot
    call check_cpuid
    call check_long_mode

    call setup_page_tables
    call enable_paging

    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:long_mode_start
    


; ============================================================================
; Validation Routines
; =========================================================================

check_multiboot:
    cmp eax, 0x36d76289
    jne .no_multiboot
    ret
.no_multiboot:
    mov bl, "M"                 
    jmp trigger_error

check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    mov bl, "C"
    jmp trigger_error

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jz .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode

    ret
.no_long_mode:      ; I get error here
    mov bl, "L"
    jmp trigger_error




setup_page_tables:
    mov eax, page_table_l3
    or eax, 0b11    ; Present, writable
    mov [page_table_l4], eax
    mov ecx, 0  ; counter
.link_l3_loop:
    mov eax, 4096                       ; size of 1 table
    mul ecx                             ; ofset for current l2
    add eax, page_table_l2              ; physical base of l2 array
    or eax, 0b11                        ; preesent + writable
    mov [page_table_l3 + ecx * 8], eax  ; linke to l3 array 

    inc ecx
    cmp ecx, 8      ; like all 8 tables (8gib)
    jne .link_l3_loop

    ; map 4096 huge maps (2MiB)
    mov ecx, 0
.map_l2_loop:
    ; calc physical address: eax = ecx * 2mib
    mov eax, 0x200000   ; 2mib in bytes
    mul ecx             ; edx:eax = ecx * 2mib

    ; for >4gb maps, physical addresses exceed 32bits
    ; edx holds upper 32bit, eax holds lower 32
    or eax, 0x83        ; Present + Writable + Huge page (bit 7)

    ; write 64bit page table into mem
    mov [page_table_l2 + ecx * 8], eax             ; lower 32 bits + flags
    mov [page_table_l2 + ecx * 8 + 4], edx         ; upper 32 bits (holds bits 32 - 63 of address)

    inc ecx
    cmp ecx, 4096       ; 512 entries * 8 tables = 4096 total pagres
    jne .map_l2_loop
    ret



enable_paging:
    ; pass table location into cpu
    mov eax, page_table_l4
    mov cr3, eax

    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; enable long-john silver mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret
   

; ==================================================================
; Error Handling & Printing Routines
; ============================================================================
global trigger_error

; Input: BL = ASCII error code character (e.g., 'M', 'C', 'L')
trigger_error:
    ; 1. Insert the error code from BL into the template placeholder
    mov [error_template + 5], bl

    ; 2. Point ESI to the newly updated string
    mov esi, error_template

    ; 3. Call your 32-bit print routine
    call puts_32
    
    ; 4. Halt the CPU (system crashed)
    cli
    hlt

; Expects: ESI = pointer to string
puts_32:
    mov edi, 0xB8000            ; VGA text buffer address

.loop:
    mov al, [esi]               ; Get character from string
    or al, al                   ; Check for null-terminator (\0)
    jz .done

    mov [edi], al               ; Write character byte
    mov byte [edi + 1], 0x4F    ; Write attribute (White text on Red background)

    inc esi                     ; Advance string pointer
    add edi, 2                  ; Advance VGA buffer position
    jmp .loop

.done:
    ret

; ================================================================
; Data & Storage Sections
;============================================================================
section .data
    ; The 5th character (index 5) is placeholder 'X'
    error_template: db "ERR: X", 0

section .bss
align 4096                        ; System V ABI requires 16-byte stack alignment me thinks
page_table_l4:
    resb 4096
page_table_l3:
    resb 4096
page_table_l2:
    resb 4096 * 8   ; 8 l2s for 8gb

stack_bum:
    resb 4096 * 4
stack_top:



section .rodata

gdt64:
    dq 0 ; entry
.code_segment: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; code segment
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

