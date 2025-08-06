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
global peachos_read:function

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

; long peachos_read(void* buffer, size_t size, size_t count, long fd);
peachos_read:
    mov rax, 12 ; Command 12 fread
    push qword rcx ; fd
    push qword rdx ; count
    push qword rsi ; size
    push qword rdi ; buffer
    int 0x80  ; invoke kernel
    add rsp, 32 ; restore the stack
    ret