#include "elf.h"
#include <stdint.h>
#include <stddef.h>

void* elf_get_entry_ptr(struct elf_header* elf_header)
{
    return (void*) (uintptr_t)elf_header->e_entry;
}

elf64_addr elf_get_entry(struct elf_header* elf_header)
{
    return elf_header->e_entry;
}