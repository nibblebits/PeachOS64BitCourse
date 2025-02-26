[BITS 64]

section .asm

global paging_load_directory
global paging_invalidate_tlb_entry

; void paging_load_directory(uintptr_t* directory)
paging_load_directory:
    mov rax, rdi  ; Load the first argument (directory) into RAX
    mov cr3, rax  ; LOad the page tables PML4 into CR3
    ret

; void paging_invalidate_tlb_entry(void* addr)
paging_invalidate_tlb_entry:
    invlpg [rdi]
    ret