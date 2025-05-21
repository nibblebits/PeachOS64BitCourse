[BITS 64]

section .asm

global paging_load_directory
global paging_invalidate_tlb_entry


; void paging_load_directory(uint64_t *directory)
paging_load_directory:
    mov rax, rdi       ; Load the first argument (directory) into RAX
    mov cr3, rax       ; Load the page directory base address into CR3
    ret

; void paging_invalidate_tlb_entry(void* addr)
; Invalidate the TLB entry for the given virtual address. requiring the CPU to check the page tables when the address
; is accessed again.
paging_invalidate_tlb_entry:
    invlpg [rdi] ; Invalidate the TLB entry for the given address.
    ret