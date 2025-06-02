[BITS 64]
section .asm

global tss_load

tss_load:
    xor rax, rax
    mov ax, di
    ltr ax
    ret
    
