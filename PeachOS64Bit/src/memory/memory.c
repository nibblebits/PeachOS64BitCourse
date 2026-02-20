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

#include "memory.h"
#include "config.h"

size_t e820_total_entries()
{
    return *((uint16_t*) PEACHOS_MEMORY_MAP_TOTAL_ENTRIES_LOCATION);
}

struct e820_entry* e820_entry(size_t index)
{
    if (index >= e820_total_entries())
    {
        return NULL;
    }

    struct e820_entry* entries = (struct e820_entry*) PEACHOS_MEMORY_MAP_LOCATION;
    return &entries[index];
}

struct e820_entry* e820_largest_free_entry()
{
    size_t total_memory_entries = e820_total_entries();
    struct e820_entry* entries = (struct e820_entry*) PEACHOS_MEMORY_MAP_LOCATION;

    // We only care about long continuous memory regions
    struct e820_entry* chosen_entry = NULL;
    for (int i = 0; i < total_memory_entries; i++)
    {
        struct e820_entry* entry = &entries[i];
        if (entry->type == 1)
        {
            // Usable memory
            if (chosen_entry == NULL)
            {
                chosen_entry = entry;
                continue;
            }

            if (entry->length > chosen_entry->length)
            {
                chosen_entry = entry;
            }
        }
    }

    return chosen_entry;
}

size_t e820_total_accessible_memory()
{
    size_t total_memory_entries = e820_total_entries();
    struct e820_entry* entries = (struct e820_entry*) PEACHOS_MEMORY_MAP_LOCATION;

    size_t total_memory = 0;
    for(int i = 0; i < total_memory_entries; i++)
    {
        struct e820_entry* entry = &entries[i];
        if (entry->type == 1)
        {
            // usable memory
            total_memory += entry->length;
        }
    }

    return total_memory;
}

void* memset(void* ptr, int c, size_t size)
{
    char* c_ptr = (char*) ptr;
    for (int i = 0; i < size; i++)
    {
        c_ptr[i] = (char) c;
    }
    return ptr;
}

int memcmp(void* s1, void* s2, int count)
{
    char* c1 = s1;
    char* c2 = s2;
    while(count-- > 0)
    {
        if (*c1++ != *c2++)
        {
            return c1[-1] < c2[-1] ? -1 : 1;
        }
    }

    return 0;
}

void* memcpy(void* dest, void* src, int len)
{
    char *d = dest;
    char *s = src;
    while(len--)
    {
        *d++ = *s++;
    }
    return dest;
}