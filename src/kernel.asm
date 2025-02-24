[BITS 32]

global _start
extern kernel_main

; Segment Selectors
CODE_SEG equ 0x08
DATA_SEG equ 0x10
LONG_MODE_CODE_SEG equ 0x18
LONG_MODE_DATA_SEG equ 0x20 ; Selector for the 64 bit data segment (offset in gdt)

_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Setup the stack
    mov ebp, 0x00200000
    mov esp, ebp

    ; Load the global descriptor table (GDT)
    lgdt [gdt_descriptor]

    ; Enable PAE(Physical Address Extension) in CR4
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Setup the page tables
    mov eax, PML4_Table
    mov cr3, eax

    ; IA32_EFER 
    mov ecx, 0xC0000080 ; IA32_EFER Index
    rdmsr               ; Reads IA32_EFER_MSR Into EDX:EAX
    or eax, 0x100       ; (Long Mode Enable) bit (bit 8)
    wrmsr               ; Write back to IA32_EFER MSR

    ; Enable paging in CR0
    mov eax, cr0
    or eax, 1 << 31     ; Set PG bit (bit 31)
    mov cr0, eax

    ; JMP TO 64 BIT MODE
    jmp LONG_MODE_CODE_SEG:long_mode_entry

[BITS 64]
long_mode_entry:
    mov ax, LONG_MODE_DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Set up the stack pointer RSP
    ; (HIGH 32 BITS ARE NEW | ESP (32 bits) )
    mov rsp, 0x00200000
    mov rbp, rsp
    jmp kernel_main


    jmp $


; Global descriptor table (GDT)
align 8 
gdt: 
    ; Null descriptor (required)
    dq 0x0000000000000000 

    ; 32-Bit code segment descriptor
    dw 0xffff ; Segment limit 0-15 bits
    dw 0    ; Base first 0-15 bits
    db 0    ; Base 16-23 bits
    db 0x9a ; Access byte
    db 11001111b ; HIgh 4 bit flags and the low 4 bit flags
    db 0         ; Base 24-31 bits


    ; 32 bit Data segment descriptor
    dw 0xffff   ; Segment limit first 0-15 bits
    dw 0        ; Base first 0-15 bits
    db 0        ; Base 16-23 bits
    db 0x92     ; Access byte
    db 11001111b ; High  bit flags and low 4 bit flags
    db 0        ; Base 24-31 bits

    ; 64 bit code segment descriptor
    dw 0x0000               ; Segment limit low (ignored in long mode)
    dw 0x0000               ; Base address low
    db 0x00                 ; Base address middle
    db 0x9A                 ; Access byte: Code segment, executable and eradable
    db 0x20                 ; Flag: Long MOde Segment
    db 0x00                 ; Base address high

    ; 64 bit data segment descriptor
    dw 0x0000           ; Segment limit low
    dw 0x0000           ; Base address low
    db 0x00             ; Base address middle
    db 0x92             ; Access byte data segment, read/write, present
    db 0x00             ; Long mode data segment has flag to zero
    db 0x00             ; Base address high

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt -1 ; Size of GDT minus 1
    dd gdt              ; Base address of GDT

; Page table definitions
align 4096
PML4_Table:
    dq PDPT_TABLE + 0x03    ; PML4 Entry pointing to PDPT (Present, RW)
    times 511 dq 0          ; Null the remaining entries

align 4096
PDPT_TABLE:
    dq PD_Table + 0x03      ; PDPT entry pointing to PD(Present, RW)
    times 511 dq 0          ; Remaining entries to be set to zero

%define PS_FLAG 0x83        ; Page size flag for 2 Mib bytes
%define PAGE_INCREMENT 0x200000

align 4096
PD_Table:
    %assign addr 0x0000000 ; Start address
    %rep 65                 ; Number of pages
        dq addr + PS_FLAG
        %assign addr addr + PAGE_INCREMENT
    %endrep
    times 509 dq 0              ; Remaining entries to zero