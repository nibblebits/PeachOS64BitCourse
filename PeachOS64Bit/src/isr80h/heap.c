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

#include "heap.h"
#include "task/task.h"
#include "task/process.h"
#include <stddef.h>

void* isr80h_command15_realloc(struct interrupt_frame* frame)
{
    void* userland_virt_addr = (void*) task_get_stack_item(task_current(), 0);
    void* new_alloc_addr = NULL;
    size_t new_ptr_size = (size_t) task_get_stack_item(task_current(), 1);
    new_alloc_addr = process_realloc(task_current()->process, userland_virt_addr, new_ptr_size);
    return new_alloc_addr;
}

void* isr80h_command4_malloc(struct interrupt_frame* frame)
{
    size_t size = (uintptr_t)task_get_stack_item(task_current(), 0);
    return process_malloc(task_current()->process, size);
}


void* isr80h_command5_free(struct interrupt_frame* frame)
{
    void* ptr_to_free = task_get_stack_item(task_current(), 0);
    process_free(task_current()->process, ptr_to_free);
    return 0;
}