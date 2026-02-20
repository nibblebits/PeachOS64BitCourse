; PeachOS 64-Bit Kernel Project
; Copyright (C) 2026 Daniel McCarthy <daniel@dragonzap.com>
;
; This file is part of the PeachOS 64-Bit Kernel.
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; version 2 as published by the Free Software Foundation.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
; See the GNU General Public License version 2 for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, see <https://www.gnu.org/licenses/>.
;
; For full source code, documentation, and structured learning,
; see the official kernel development course part one:
; https://dragonzap.com/course/developing-a-multithreaded-kernel-from-scratch
;
; Get part one and part two module one, module two all peachos courses (69 hours of content): https://dragonzap.com/offer/kernel-development-from-scratch-69-hours
;
; Get the part two, module one and two modules: https://dragonzap.com/offer/developing-a-multithreaded-kernel-from-scratch-part-two-full-series
;

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