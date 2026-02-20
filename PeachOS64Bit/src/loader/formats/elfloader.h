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

#ifndef ELFLOADER_H
#define ELFLOADER_H

#include <stdint.h>
#include <stddef.h>

#include "elf.h"
#include "config.h"

struct elf_file
{
    char filename[PEACHOS_MAX_PATH];

    int in_memory_size;

    /**
     * The physical memory address that this elf file is loaded at
     */
    void* elf_memory;

    /**
     * The virtual base address of this binary
     */
    void* virtual_base_address;

    /**
     * The ending virtual address
     */
    void* virtual_end_address;

    /**
     * The physical base address of this binary
     */
    void* physical_base_address;

    /**
     * The physical end address of this bunary
     */
    void* physical_end_address;
    

};

int elf_load(const char* filename, struct elf_file** file_out);
struct elf_file* elf_file_new();
void elf_file_free(struct elf_file* file);

void elf_close(struct elf_file* file);
void* elf_virtual_base(struct elf_file* file);
void* elf_virtual_end(struct elf_file* file);
void* elf_phys_base(struct elf_file* file);
void* elf_phys_end(struct elf_file* file);

struct elf_header* elf_header(struct elf_file* file);
struct elf64_shdr* elf_sheader(struct elf_header* header);
void* elf_memory(struct elf_file* file);
struct elf64_phdr* elf_pheader(struct elf_header* header);
struct elf64_phdr* elf_program_header(struct elf_header* header, int index);
struct elf64_shdr* elf_section(struct elf_header* header, int index);
void* elf_phdr_phys_address(struct elf_file* file, struct elf64_phdr* phdr);

#endif