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

#include "elfloader.h"
#include "fs/file.h"
#include "status.h"
#include <stdbool.h>
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "string/string.h"
#include "memory/paging/paging.h"
#include "kernel.h"
#include "config.h"


const char elf_signature[] = {0x7f, 'E', 'L', 'F'};

static bool elf_valid_signature(void* buffer)
{
    return memcmp(buffer, (void*) elf_signature, sizeof(elf_signature)) == 0;
}

static bool elf_valid_class(struct elf_header* header)
{
    // We only support 64 bit binaries.
    return header->e_ident[EI_CLASS] == ELFCLASSNONE || header->e_ident[EI_CLASS] == ELFCLASS64;
}

static bool elf_valid_encoding(struct elf_header* header)
{
    return header->e_ident[EI_DATA] == ELFDATANONE || header->e_ident[EI_DATA] == ELFDATA2LSB;
}

static bool elf_is_executable(struct elf_header* header)
{
    return header->e_type == ET_EXEC && header->e_entry >= PEACHOS_PROGRAM_VIRTUAL_ADDRESS;
}

static bool elf_has_program_header(struct elf_header* header)
{
    return header->e_phoff != 0;
}

void* elf_memory(struct elf_file* file)
{
    return file->elf_memory;
}

struct elf_header* elf_header(struct elf_file* file)
{
    return file->elf_memory;
}

struct elf64_shdr* elf_sheader(struct elf_header* header)
{
    return (struct elf64_shdr*)((elf64_off)header+header->e_shoff);
}

struct elf64_phdr* elf_pheader(struct elf_header* header)
{
    if(header->e_phoff == 0)
    {
        return 0;
    }

    return (struct elf64_phdr*)((elf64_off)header + header->e_phoff);
}

struct elf64_phdr* elf_program_header(struct elf_header* header, int index)
{
    return &elf_pheader(header)[index];
}

struct elf64_shdr* elf_section(struct elf_header* header, int index)
{
    return &elf_sheader(header)[index];
}


void* elf_phdr_phys_address(struct elf_file* file, struct elf64_phdr* phdr)
{
    return elf_memory(file)+phdr->p_offset;
}

char* elf_str_table(struct elf_header* header)
{
    return (char*) header + elf_section(header, header->e_shstrndx)->sh_offset;
}

void* elf_virtual_base(struct elf_file* file)
{
    return file->virtual_base_address;
}

void* elf_virtual_end(struct elf_file* file)
{
    return file->virtual_end_address;
}

void* elf_phys_base(struct elf_file* file)
{
    return file->physical_base_address;
}

void* elf_phys_end(struct elf_file* file)
{
    return file->physical_end_address;
}

int elf_validate_loaded(struct elf_header* header)
{
    return (elf_valid_signature(header) && elf_valid_class(header) && elf_valid_encoding(header) && elf_has_program_header(header) && elf_is_executable(header)) ? PEACHOS_ALL_OK : -EINFORMAT;
}

int elf_process_phdr_pt_load(struct elf_file* elf_file, struct elf64_phdr* phdr)
{
    if (elf_file->virtual_base_address >= (void*) phdr->p_vaddr || elf_file->virtual_base_address == 0x00)
    {
        elf_file->virtual_base_address = (void*)  phdr->p_vaddr;
        elf_file->physical_base_address = elf_memory(elf_file)+phdr->p_offset;
    }

    uintptr_t end_virtual_address = phdr->p_vaddr + phdr->p_filesz;
    if (elf_file->virtual_end_address <= (void*)(end_virtual_address) || elf_file->virtual_end_address == 0x00)
    {
        elf_file->virtual_end_address = (void*) end_virtual_address;
        elf_file->physical_end_address = elf_memory(elf_file)+phdr->p_offset+phdr->p_filesz;
    }

    size_t filesize = phdr->p_filesz;
    size_t total_size = phdr->p_memsz;
    if (total_size > filesize)
    {
        memset((char*)elf_file->physical_base_address+filesize, 0, total_size-filesize);
    }
    return 0;
}
int elf_process_pheader(struct elf_file* elf_file, struct elf64_phdr* phdr)
{
    int res = 0;
    switch(phdr->p_type)
    {
        case PT_LOAD:
            res = elf_process_phdr_pt_load(elf_file, phdr);
        break;
    }
    return res;
}
int elf_process_pheaders(struct elf_file* elf_file)
{
    int res = 0;
    struct elf_header* header = elf_header(elf_file);
    for(int i = 0; i < header->e_phnum; i++)
    {
        struct elf64_phdr* phdr = elf_program_header(header, i);
        res = elf_process_pheader(elf_file, phdr);
        if (res < 0)
        {
            break;
        }

    }
    return res;
}

int elf_process_loaded(struct elf_file* elf_file)
{
    int res = 0;
    struct elf_header* header = elf_header(elf_file);
    res = elf_validate_loaded(header);
    if (res < 0)
    {
        goto out;
    }

    res = elf_process_pheaders(elf_file);
    if (res < 0)
    {
        goto out;
    }

out:
    return res;
}

void elf_file_free(struct elf_file* elf_file)
{
    if (elf_file->elf_memory)
    {
        kfree(elf_file->elf_memory);
    }

    kfree(elf_file);
}
struct elf_file* elf_file_new()
{
    return (struct elf_file*)kzalloc(sizeof(struct elf_file));
}

int elf_load(const char* filename, struct elf_file** file_out)
{
    struct elf_file* elf_file = elf_file_new();
    int fd = 0;
    int res = fopen(filename, "r");
    if (res <= 0)
    {
        res = -EIO;
        goto out;
    }

    fd = res;
    struct file_stat stat;
    res = fstat(fd, &stat);
    if (res < 0)
    {
        goto out;
    }

    elf_file->elf_memory = kzalloc(stat.filesize);
    res = fread(elf_file->elf_memory, stat.filesize, 1, fd);
    if (res < 0)
    {
        goto out;
    }

    res = elf_process_loaded(elf_file);
    if(res < 0)
    {
        goto out;
    }

    *file_out = elf_file;
out:
    if (res < 0)
    {
        elf_file_free(elf_file);
    }
    fclose(fd);
    return res;
}

void elf_close(struct elf_file* file)
{
    if (!file)
        return;

    kfree(file->elf_memory);
    kfree(file);
}