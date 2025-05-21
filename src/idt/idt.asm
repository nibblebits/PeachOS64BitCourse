[BITS 64]
section .asm

; MUST NOW CONVERT TO 64 BIT COMPATIBLE CODE
extern int21h_handler
extern no_interrupt_handler
extern isr80h_handler
extern interrupt_handler

global idt_load
global no_interrupt
global enable_interrupts
global disable_interrupts
global isr80h_wrapper
global interrupt_pointer_table

temp_rsp_storage: dq 0x00


;; NO SUPPORT FOR PUSHAD OR POPAD IN 64 BIT LONG MODE WE NEED TO MAKE IT OURSELVES.
%macro pushad_macro 0
        mov qword [temp_rsp_storage], rsp
        push rax
        push rcx
        push rdx
        push rbx
        push qword [temp_rsp_storage]
        push rbp
        push rsi
        push rdi
%endmacro

%macro popad_macro 0
    pop rdi
    pop rsi
    pop rbp
    pop qword [temp_rsp_storage]
    pop rbx
    pop rdx
    pop rcx
    pop rax
    mov rsp, [temp_rsp_storage]
%endmacro


enable_interrupts:
    sti
    ret

disable_interrupts:
    cli
    ret


idt_load:
    mov rbx, rdi
    lidt [rbx]
    ret


no_interrupt:
    pushad_macro
    call no_interrupt_handler
    popad_macro
    iretq





%macro interrupt 1
    global int%1
    int%1:
        ; INTERRUPT FRAME START
        ; ALREADY PUSHED TO US BY THE PROCESSOR UPON ENTRY TO THIS INTERRUPT
        ; uint64_t ip
        ; uint64_t cs;
        ; uint64_t flags
        ; uint64_t sp;
        ; uint64_t ss;
        ; Pushes the general purpose registers to the stack
        ; pushad
        ; pushad does not work in 64 bi tmore we will need to manually do it
        ;Temp := (ESP);
        ;Push(EAX);
        ;Push(ECX);
        ;Push(EDX);
        ;Push(EBX);
        ;Push(Temp);
        ;Push(EBP);
        ;Push(ESI);
        ;Push(EDI);

        ; EQUIVILANT TO PUSHAD IF IT WAS SUPPORTED IN 64 BIT MODE
        pushad_macro
        ; END PUSHAD

        ; Interrupt frame end
        mov rdi, %1
        ; RSP points to the top of the PUSHAD stack
        mov rsi, rsp
        ; should work in 64 bit mode now.
        call interrupt_handler
        popad_macro
        iretq
%endmacro


%assign i 0
%rep 512
    interrupt i
%assign i i+1
%endrep

isr80h_wrapper:
    ; INTERRUPT FRAME START
    ; ALREADY PUSHED TO US BY THE PROCESSOR UPON ENTRY TO THIS INTERRUPT
    ; uint64_t ip
    ; uint64_t cs;
    ; uint64_t flags
    ; uint64_t sp;
    ; uint64_t ss;
    ; Pushes the general purpose registers to the stack
    pushad_macro
    
    ; INTERRUPT FRAME END

    ; Second argument is the interrupt stack pointer.
    mov rsi, rsp

    ; RAX holds our first argument, the command lets prepare for isr80h_handler
    mov rdi, rax
    call isr80h_handler
    mov qword[tmp_res], rax

    ; Restore general purpose registers for user land
    popad_macro
    mov rax, [tmp_res]
    iretq

section .data
; Inside here is stored the return result from isr80h_handler
tmp_res: dq 0


%macro interrupt_array_entry 1
    dq int%1
%endmacro


; Every value in this array is zero for some reason, leading to the problem we face
; must be an addressing problem
interrupt_pointer_table:
%assign i 0
%rep 512
    interrupt_array_entry i
%assign i i+1
%endrep