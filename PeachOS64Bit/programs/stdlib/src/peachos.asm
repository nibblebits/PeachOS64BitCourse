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

global print:function
global peachos_getkey:function
global peachos_malloc:function
global peachos_free:function
global peachos_putchar:function
global peachos_process_load_start:function
global peachos_process_get_arguments:function 
global peachos_system:function
global peachos_exit:function
global peachos_fopen:function
global peachos_fclose:function
global peachos_fread:function
global peachos_fseek:function
global peachos_fstat:function
global peachos_realloc:function

; void print(const char* filename)
print:
    push qword rdi
    mov rax, 1 ; Command print
    int 0x80
    add rsp, 8
    ret

; int peachos_getkey()
peachos_getkey:
    mov rax, 2 ; Command getkey
    int 0x80
    ret

; void peachos_putchar(char c)
peachos_putchar:
    mov rax, 3 ; Command putchar
    push qword rdi ; Variable "c"
    int 0x80
    add rsp, 8
    ret

; void* peachos_malloc(size_t size)
peachos_malloc:
    mov rax, 4 ; Command malloc (Allocates memory for the process)
    push qword rdi ; Variable "size"
    int 0x80
    add rsp, 8
    ret

; void peachos_free(void* ptr)
peachos_free:
    mov rax, 5 ; Command 5 free (Frees the allocated memory for this process)
    push qword rdi ; Variable "ptr"
    int 0x80
    add rsp, 8
    ret

; void peachos_process_load_start(const char* filename)
peachos_process_load_start:
    mov rax, 6 ; Command 6 process load start ( stars a process )
    push qword rdi ; Variable "filename"
    int 0x80
    add rsp, 8
    ret

; int peachos_system(struct command_argument* arguments)
peachos_system:
    mov rax, 7 ; Command 7 process_system ( runs a system command based on the arguments)
    push qword rdi  ; Variable "arguments"
    int 0x80
    add rsp, 8
    ret


; void peachos_process_get_arguments(struct process_arguments* arguments)
peachos_process_get_arguments:
    mov rax, 8 ; Command 8 Gets the process arguments
    push qword rdi ; Variable arguments
    int 0x80
    add rsp, 8
    ret

; void peachos_exit()
peachos_exit:
    mov rax, 9 ; Command 9 process exit
    int 0x80
    ret

; int peachos_fopen(const char* filename, const char* mode)

peachos_fopen:
    mov rax, 10 ; Command 10, fopen
    push qword rsi   ; Pushes the mode 
    push qword rdi   ; Push the filename
    int 0x80        ; call the kernel
    add rsp, 16 ; restore the stack
    ret

; void peachos_fclose(size_t fd);
peachos_fclose:
    mov rax, 11 ; Command 11 fclose
    push qword rdi 
    add rsp, 8  ; restore the stack
    ret

; long peachos_fread(void* buffer, size_t size, size_t count, long fd);
peachos_fread:
    mov rax, 12 ; Command 12 fread
    push qword rcx ; fd
    push qword rdx ; count
    push qword rsi ; size
    push qword rdi ; buffer
    int 0x80  ; invoke kernel
    add rsp, 32 ; restore the stack
    ret

; long peachos_fseek(long fd, long offset, long whence);
peachos_fseek:
    mov rax, 13 ; Command 13 fseek 
    push qword rdx ; whence
    push qword rsi ; offset
    push qword rdi ; fd
    int 0x80       ; invokes the kernel
    add rsp, 24    ; restores the stack
    ret            ; return

; long peachos_fstat(long fd, struct file_stat* file_stat_out)
peachos_fstat:
    mov rax, 14    ; Command 14 fstat
    push qword rsi  ; file_stat_out
    push qword rdi  ; fd
    int 0x80        ; call kernel
    add rsp, 16     ; restore stack
    ret

; void* peachos_realloc(void* old_ptr, size_t new_size);
peachos_realloc:
    mov rax, 15     ; Command 15 realloc
    push qword rsi  ; new_size
    push qword rdi  ; old_ptr
    int 0x80
    add rsp, 16
    ; RAX = new the pointer address
    ret