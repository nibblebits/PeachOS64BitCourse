[BITS 32]

global _start
global kernel_registers
global div_test
global gdt
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
kernel_registers:
    mov ax, LONG_MODE_DATA_SEG
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    ret

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



; Remap the master PIC
    mov al, 00010001b      ; ICW1: start initialization in cascade mode
    out 0x20, al           ; send ICW1 to master command port

    mov al, 0x20           ; ICW2: master PIC vector offset (0x20)
    out 0x21, al           ; send ICW2 to master data port

    mov al, 0x04           ; ICW3: tell Master PIC that there is a slave PIC at IRQ2 (binary 0000 0100)
    out 0x21, al           ; send ICW3 to master data port

    mov al, 00000001b      ; ICW4: set environment info (8086/88 mode)
    out 0x21, al           ; send ICW4 to master data port

    ; Remap the slave PIC
    mov al, 00010001b      ; ICW1: start initialization in cascade mode
    out 0xA0, al           ; send ICW1 to slave command port

    mov al, 0x28           ; ICW2: slave PIC vector offset (0x28)
    out 0xA1, al           ; send ICW2 to slave data port

    mov al, 0x02           ; ICW3: tell Slave PIC its cascade identity (connected to master's IRQ2)
    out 0xA1, al           ; send ICW3 to slave data port

    mov al, 00000001b      ; ICW4: set environment info (8086/88 mode)
    out 0xA1, al           ; send ICW4 to slave data port

    ; End remap of the master PIC

    jmp kernel_main


    jmp $


div_test:
    mov rax, 0
    idiv rax 
    ret 

align 8
gdt:
    ; Null Descriptor (Required)
    dq 0x0000000000000000

    ; 32-bit Code Segment Descriptor
    dw 0xffff ; Segment limit first 0-15 bits
    dw 0      ; Base first 0-15 bits
    db 0      ; Base 16-23 bits
    db 0x9a   ; Access byte
    db 11001111b ; High 4 bit flags and the low 4 bit flags
    db 0        ; Base 24-31 bits


    ; 32-bit Data Segment Descriptor
    dw 0xffff ; Segment limit first 0-15 bits
    dw 0      ; Base first 0-15 bits
    db 0      ; Base 16-23 bits
    db 0x92   ; Access byte
    db 11001111b ; High 4 bit flags and the low 4 bit flags
    db 0        ; Base 24-31 bits

    ; 64-bit Code Segment Descriptor
    dw 0x0000                  ; Segment Limit Low (ignored in long mode)
    dw 0x0000                  ; Base Address Low
    db 0x00                    ; Base Address Middle
    db 0x9A                    ; Access Byte: Code Segment, Executable, Readable
    db 0x20                    ; Flags: Long Mode Segment
    db 0x00                    ; Base Address High

    ; 64-bit Data Segment Descriptor
    dw 0x0000                  ; Segment Limit Low
    dw 0x0000                  ; Base Address Low
    db 0x00                    ; Base Address Middle
    db 0x92                    ; Access Byte: Data Segment, Read/Write, Present
    db 0xC0                    ; Flags: Long mode data segments have flags set to 0
    db 0x00                    ; Base Address High

    ; 64-bit User Code Segment Descriptor
    dw 0x0000                  ; Segment Limit Low (ignored in long mode)
    dw 0x0000                  ; Base Address Low
    db 0x00                    ; Base Address Middle
    db 0xFA                    ; Access Byte: Code Segment, Executable, Readable, User Mode
    db 0x20                    ; Flags: Long Mode Segment
    db 0x00                    ; Base Address High

    ; 64-bit User Data Segment Descriptor
    dw 0x0000                  ; Segment Limit Low
    dw 0x0000                  ; Base Address Low
    db 0x00                    ; Base Address Middle
    db 0xF2                    ; Access Byte: Data Segment, Read/Write, User Mode
    db 0x00                    ; Flags: Long mode data segments have flags set to 0
    db 0x00                    ; Base Address High


   ; TSS IS IN TWO ENTRIES FOR 64 BIT MODE
   ; 64-bit TSS  Segment Descriptor
   ; NULL as it will be initialized in the C code.
    dw 0x0000                  ; Segment Limit Low
    dw 0x0000                  ; Base Address Low
    db 0x00                    ; Base Address Middle
    db 0x00                    ; Access Byte: Data Segment, Read/Write, User Mode
    db 0x00                    ; Flags: Long mode data segments have flags set to 0
    db 0x00                    ; Base Address High


   ; 64-bit TSS  Segment Descriptor 2
   ; NULL as it will be initialized in the C code.
    dw 0x0000                  ; Segment Limit Low
    dw 0x0000                  ; Base Address Low
    db 0x00                    ; Base Address Middle
    db 0x00                    ; Access Byte: Data Segment, Read/Write, User Mode
    db 0x00                    ; Flags: Long mode data segments have flags set to 0
    db 0x00                    ; Base Address High


gdt_end:

gdt_descriptor:
    dw gdt_end - gdt - 1    ; 2 bytes: Limit
    dq gdt                  ; 8 bytes: 64-bit Base address

align 4096
PML4_Table:
    dq PDPT_Table + 0x03       ; PML4 entry pointing to PDPT (Present, RW)
    times 511 dq 0             ; Remaining entries set to zero

align 4096
PDPT_Table:
    dq PD_Table + 0x03         ; PDPT entry pointing to PD (Present, RW)
    times 511 dq 0             ; Remaining entries set to zero

align 4096
; Map up to 1 GB of memory. Doesn't matter if theres not enough ram for that
; the mapping is temporary until we can reinitialize the page tables on the C code side.
PD_Table:
    %assign addr 0x00000000    ; Start address
    %rep 512                   ; Number of entries in a Page Directory
        dq addr + 0x83         ; PD entry mapping 2 MiB page (Present, RW, PS)
        %assign addr addr + 0x200000 ; Increment by 2 MiB
    %endrep



