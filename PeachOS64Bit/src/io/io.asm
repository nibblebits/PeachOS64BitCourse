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