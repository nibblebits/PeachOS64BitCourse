[BITS 64]
section .asm

global tss_load

tss_load:
    mov ax, di ; TSS Segment
    ltr ax
    ret