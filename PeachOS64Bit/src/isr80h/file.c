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

#include "file.h"
#include "task/task.h"
#include "task/process.h"
#include "idt/idt.h"
#include <stddef.h>
#include <stdint.h>

void* isr80h_command14_fstat(struct interrupt_frame* frame)
{
    long fd = (long) task_get_stack_item(task_current(), 0);
    struct file_stat* virt_file_stat_addr = (struct file_stat*) task_get_stack_item(task_current(), 1);
    return (void*)(long) process_fstat(task_current()->process, fd, virt_file_stat_addr);
}

void* isr80h_command13_fseek(struct interrupt_frame* frame)
{ 
    long fd = (long) task_get_stack_item(task_current(), 0);
    long offset = (long) task_get_stack_item(task_current(), 1);
    long whence = (long) task_get_stack_item(task_current(), 2);

    return (void*) (long) process_fseek(task_current()->process, fd, offset, whence);
}

void* isr80h_command12_fread(struct interrupt_frame* frame)
{
    int res = 0;
    void* buffer_virt_addr = task_get_stack_item(task_current(), 0);
    size_t size = (size_t) task_get_stack_item(task_current(), 1);
    size_t count = (size_t) task_get_stack_item(task_current(), 2);

    long fd = (long) task_get_stack_item(task_current(), 3);
    res = process_fread(task_current()->process, buffer_virt_addr, size, count, fd);
    return (void*) (int64_t) res;
}

void* isr80h_command11_fclose(struct interrupt_frame* frame)
{
    int64_t fd = 0;
    fd = (int64_t) task_get_stack_item(task_current(), 0);

    // We have the file number lets close it
    process_fclose(task_current()->process, fd);

    // return null
    return NULL;
}

void* isr80h_command10_fopen(struct interrupt_frame* frame)
{
    int fd = 0;
    void* filename_virt_addr = NULL;
    void* mode_virt_addr = NULL;
    filename_virt_addr = task_get_stack_item(task_current(), 0);
    filename_virt_addr = task_virtual_address_to_physical(task_current(), filename_virt_addr);
    if (!filename_virt_addr)
    {
        fd = -1;
        goto out;
    }

    mode_virt_addr = task_get_stack_item(task_current(), 1);
    mode_virt_addr = task_virtual_address_to_physical(task_current(), mode_virt_addr);
    if (!mode_virt_addr)
    {
        fd = -1;
        goto out;
    }

    fd = process_fopen(task_current()->process, filename_virt_addr, mode_virt_addr);
    if (fd <= 0)
    {
        goto out;
    }

out:
    return (void*) (int64_t) fd;
}