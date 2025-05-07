[BITS 64]
section .asm

global restore_general_purpose_registers
global task_return
global user_registers

; void task_return(struct registers* regs);
task_return:
    push qword [rdi+88] ; SS
    push qword [rdi+80] ; RSP
    mov rax, [rdi+72]   ; RFLAGS
    or rax, 0x200       ; Set IF Bit
    push rax

    push qword 0x2B     ; User data segment
    push qword [rdi+56] ; RIP
    call restore_general_purpose_registers

    ; Leave the kernel and jump to user land.
    iretq 
    
; void restore_general_purpose_registers(struct registers* regs);
restore_general_purpose_registers:
    mov rsi, [rdi+8]
    mov rbp, [rdi+16]
    mov rbx, [rdi+24]
    mov rdx, [rdi+32]
    mov rcx, [rdi+40]
    mov rax, [rdi+48]

    ; Finally RDI
    mov rdi, [rdi]
    ret

; void user_registers()
user_registers:
    mov ax, 0x2B ; User data segment | privilaged bit
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret