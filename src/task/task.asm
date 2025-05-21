[BITS 64]
section .asm

global restore_general_purpose_registers
global task_return
global user_registers

; void task_return(struct registers* regs);
task_return:
    ; PUSH THE DATA SEGMENT (SS WILL BE FINE)
    ; PUSH THE STACK ADDRESS
    ; PUSH THE FLAGS
    ; PUSH THE CODE SEGMENT
    ; PUSH IP

    ; Let's access the structure passed to us

    push qword [rdi + 88]  ; SS
    push qword [rdi + 80]  ; RSP

    mov rax, [rdi + 72]    ; RFLAGS
    or rax, 0x200          ; Set IF bit
    push rax

    ;push qword [rdi + 64]  ; CS
    ; we will try manually defined
    push qword 0x2B
    push qword [rdi + 56]  ; RIP

    ;; DOnt think you should do this on 64 bit mode REMINDER!!!
    mov ax, 0x33
    mov ds, ax
    mov es, ax
    mov fs, ax
   ; stack segment is already handled abvoe.. line 18.


   
    call restore_general_purpose_registers

    ; Let's leave kernel land and execute in user land!
    iretq
    
; void restore_general_purpose_registers(struct registers* regs);
;     uint64_t rdi;
;    uint64_t rsi;
;    uint64_t rbp;
;    uint64_t rbx;
;    uint64_t rdx;
;    uint64_t rcx;
;    uint64_t rax;
restore_general_purpose_registers:
    ; RDI has to go last since we use it for the registers pointer
    mov rsi, [rdi+8]
    mov rbp, [rdi+16]
    mov rbx, [rdi+24]
    mov rdx, [rdi+32]
    mov rcx, [rdi+40]
    mov rax, [rdi+48]

    ; finally RDI
    mov rdi, [rdi]
    ret

; void user_registers()
user_registers:
    mov ax, 0x2B ; User data segment with privilage bit
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret