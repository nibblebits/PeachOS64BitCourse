/*
 * PeachOS 64-Bit Kernel Project
 * Copyright (C) 2026 Daniel McCarthy <daniel@dragonzap.com>
 *
 * This file is part of the PeachOS 64-Bit Kernel.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License version 2 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 * For full source code, documentation, and structured learning,
 * see the official kernel development course part one:
 * https://dragonzap.com/course/developing-a-multithreaded-kernel-from-scratch

 * Get part one and part two module one, module two all peachos courses (69 hours of content): https://dragonzap.com/offer/kernel-development-from-scratch-69-hours

 * Get the part two, module one and two modules: https://dragonzap.com/offer/developing-a-multithreaded-kernel-from-scratch-part-two-full-series
 */

#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stddef.h>

#define PF_X 0x01
#define PF_W 0x02
#define PF_R 0x04

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6

#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_SHLIB 10
#define SHT_DYNSYM 11
#define SHT_LOPROC 12
#define SHT_HIPROC 13
#define SHT_LOUSER 14
#define SHT_HIUSER 15

#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4

#define EI_NIDENT 16
#define EI_CLASS 4
#define EI_DATA 5

#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define SHN_UNDEF 0

typedef uint64_t elf64_addr;
typedef uint64_t elf64_off;
typedef uint16_t elf64_half;
typedef uint32_t elf64_word;
typedef uint32_t elf64_sword;
typedef uint64_t elf64_xword;
typedef uint64_t elf64_sxword;

struct elf64_phdr
{
    elf64_word p_type;
    elf64_word p_flags;
    elf64_off p_offset;
    elf64_addr p_vaddr;
    elf64_addr p_paddr;
    elf64_xword p_filesz;
    elf64_xword p_memsz;
    elf64_xword p_align;
} __attribute__((packed));

struct elf64_shdr
{
    elf64_word sh_name;
    elf64_xword sh_type;
    elf64_xword sh_flags;
    elf64_addr sh_addr;
    elf64_off sh_offset;
    elf64_xword sh_size;
    elf64_word sh_link;
    elf64_word sh_info;
    elf64_xword sh_addralign;
    elf64_xword sh_entsize;
} __attribute__((packed));

struct elf_header
{
    unsigned char e_ident[EI_NIDENT];
    elf64_half e_type;
    elf64_half e_machine;
    elf64_word e_version;
    elf64_addr e_entry;
    elf64_off e_phoff;
    elf64_off e_shoff;
    elf64_word e_flags;
    elf64_half e_ehsize;
    elf64_half e_phentsize;
    elf64_half e_phnum;
    elf64_half e_shentsize;
    elf64_half e_shnum;
    elf64_half e_shstrndx;
} __attribute__((packed));

struct elf64_dyn
{
    elf64_sxword d_tag;
    union 
    {
        elf64_xword d_val;
        elf64_addr d_ptr;
    } d_un;
    
} __attribute__((packed));

struct elf64_sym
{
    elf64_word st_name;
    unsigned char st_info;
    unsigned char st_other;
    elf64_half st_shndx;
    elf64_addr st_value;
    elf64_xword st_size;
} __attribute__((packed));

void* elf_get_entry_ptr(struct elf_header* elf_header);
elf64_addr elf_get_entry(struct elf_header* elf_header);

#endif