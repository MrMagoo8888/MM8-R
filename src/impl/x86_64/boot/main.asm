global header_start
global _start                   ; Usually required by linkers

section .text
[bits 32]

_start:
start:
    mov esp, stack_top

    call check_multiboot
    call check_cpuid
    call check_long_mode

    

    cli
    hlt

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
    je .no_cpu
    ret
.no_cpuid:
    mov bl, "C"
    jmp trigger_error

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jmp .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode

    ret
.no_long_mode:
    mov bl, "L"
    jmp trigger_error
   

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
align 16                        ; System V ABI requires 16-byte stack alignment me thinks
stack_bum:
    resb 4096 * 4
stack_top:
