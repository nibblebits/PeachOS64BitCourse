[BITS 64]
section .asm

global insb
global insw
global insdw
global outb
global outw
global outdw

insb:
    xor rax, rax
    mov dx, di
    in al, dx
    ret

insw:
    xor rax, rax
    mov dx, di
    in ax, dx
    ret

insdw: 
    xor rax, rax
    mov dx, di
    in eax, dx
    ret 

outb:
    mov ax, si
    mov dx, di
    out dx, al
    ret

outw:
    mov rax, rsi
    mov rdx, rdi
    out dx, ax 
    ret

outdw:
    mov rax, rsi
    mov rdx, rdi
    out dx, eax
    ret