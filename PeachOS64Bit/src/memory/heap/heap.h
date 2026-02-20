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

#ifndef HEAP_H
#define HEAP_H
#include "config.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define HEAP_BLOCK_TABLE_ENTRY_TAKEN 0x01
#define HEAP_BLOCK_TABLE_ENTRY_FREE 0x00

#define HEAP_BLOCK_HAS_NEXT 0b10000000
#define HEAP_BLOCK_IS_FIRST  0b01000000


typedef unsigned char HEAP_BLOCK_TABLE_ENTRY;

typedef void*(*HEAP_BLOCK_ALLOCATED_CALLBACK_FUNCTION)(void* ptr, size_t size);
typedef void(*HEAP_BLOCK_FREE_CALLBACK_FUNCTION)(void* ptr);

struct heap_table
{
    HEAP_BLOCK_TABLE_ENTRY* entries;
    size_t total;
};


struct heap
{
    struct heap_table* table;

    // Start address of the heap data pool
    void* saddr;

    // End address of the heap data pool
    void* eaddr;

    size_t total_blocks;
    size_t free_blocks;
    size_t used_blocks;

    // Callback function for when a block is allocated
    HEAP_BLOCK_ALLOCATED_CALLBACK_FUNCTION block_allocated_callback;

    // Calback function for a when a block is freed.
    HEAP_BLOCK_FREE_CALLBACK_FUNCTION block_free_callback;
};

void heap_callbacks_set(struct heap* heap, HEAP_BLOCK_ALLOCATED_CALLBACK_FUNCTION allocated_func, HEAP_BLOCK_FREE_CALLBACK_FUNCTION free_func);
size_t heap_allocation_block_count(struct heap* heap, void* starting_address);
int64_t heap_address_to_block(struct heap *heap, void *address);
bool heap_is_block_range_free(struct heap* heap, size_t starting_block, size_t ending_block);

int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table);
void* heap_malloc(struct heap* heap, size_t size);
void heap_free(struct heap* heap, void* ptr);
void* heap_zalloc(struct heap* heap, size_t size);

size_t heap_total_size(struct heap* heap);
size_t heap_total_available(struct heap* heap);
size_t heap_total_used(struct heap* heap);

uintptr_t heap_align_value_to_upper(uintptr_t val);
uintptr_t heap_align_value_to_lower(uintptr_t val);

bool heap_is_address_within_heap(struct heap* heap, void* ptr);
void* heap_realloc(struct heap* heap, void* old_ptr, size_t new_size);


#endif