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

#include "task.h"
#include "kernel.h"
#include "status.h"
#include "process.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "string/string.h"
#include "memory/paging/paging.h"
#include "loader/formats/elfloader.h"
#include "idt/idt.h"

// The current task that is running
struct task *current_task = 0;

// Task linked list
struct task *task_tail = 0;
struct task *task_head = 0;

int task_init(struct task *task, struct process *process);

struct task *task_current()
{
    return current_task;
}

struct task *task_new(struct process *process)
{
    int res = 0;
    struct task *task = kzalloc(sizeof(struct task));
    if (!task)
    {
        res = -ENOMEM;
        goto out;
    }

    res = task_init(task, process);
    if (res != PEACHOS_ALL_OK)
    {
        goto out;
    }


    if (task_head == 0)
    {
        task_head = task;
        task_tail = task;
        current_task = task;
        goto out;
    }

    task_tail->next = task;
    task->prev = task_tail;
    task_tail = task;

out:
    if (ISERR(res))
    {
        task_free(task);
        return ERROR(res);
    }

    return task;
}

struct task *task_get_next()
{
    if (!current_task->next)
    {
        return task_head;
    }

    return current_task->next;
}

static void task_list_remove(struct task *task)
{
    if (task->prev)
    {
        task->prev->next = task->next;
    }

    if (task == task_head)
    {
        task_head = task->next;
    }

    if (task == task_tail)
    {
        task_tail = task->prev;
    }

    if (task == current_task)
    {
        current_task = task_get_next();
    }
}

int task_free(struct task *task)
{
    task_list_remove(task);

    // Finally free the task data
    kfree(task);
    return 0;
}

void task_next()
{
    struct task* next_task = task_get_next();
    if (!next_task)
    {
        panic("No more tasks!\n");
    }

    task_switch(next_task);
    task_return(&next_task->registers);
}

int task_switch(struct task *task)
{
    current_task = task;
    paging_switch(task->process->paging_desc);
    return 0;
}

struct paging_desc* task_paging_desc(struct task* task)
{
    return task->process->paging_desc;
}

struct paging_desc* task_current_paging_desc()
{
    if (!current_task)
    {
        panic("NO task yet\n");
    }

    return task_paging_desc(current_task);
}



void task_save_state(struct task *task, struct interrupt_frame *frame)
{
    task->registers.ip = frame->ip;
    task->registers.cs = frame->cs;
    task->registers.flags = frame->flags;
    task->registers.rsp = frame->rsp;
    task->registers.ss = frame->ss;
    task->registers.rax = frame->rax;
    task->registers.rbp = frame->rbp;
    task->registers.rbx = frame->rbx;
    task->registers.rcx = frame->rcx;
    task->registers.rdi = frame->rdi;
    task->registers.rdx = frame->rdx;
    task->registers.rsi = frame->rsi;
}
int copy_string_from_task(struct task* task, void* virtual, void* phys, int max)
{
    if (max >= PAGING_PAGE_SIZE)
    {
        return -EINVARG;
    }

    int res = 0;
    char* tmp = kzalloc(max);
    if (!tmp)
    {
        res = -ENOMEM;
        goto out;
    }

    void* phys_tmp = paging_get_physical_address(kernel_desc(), tmp);
    struct paging_desc* task_desc = task_paging_desc(task);
    struct paging_desc_entry old_entry;
    memcpy(&old_entry, paging_get(task_desc, phys_tmp), sizeof(struct paging_desc_entry));

    int old_entry_flags = 0;
    old_entry_flags |= old_entry.read_write | old_entry.present | old_entry.user_supervisor;

    paging_map(task_desc, phys_tmp, phys_tmp, PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    
    // Switch to the user pages
    task_page_task(task);
    // we are now on the page tables of the task
    strncpy(tmp, virtual, max);

    // switch back to kernel page tables
    kernel_page();

    strncpy(phys, tmp, max);

    // Remap back to what is was before.
    paging_map(task_desc, phys_tmp, (void*)((uint64_t)(old_entry.address << 12)), old_entry_flags);
out:
    // No longer do we need the temp variable
    if (tmp)
    {
        kfree(tmp);
    }
    return res;
}
void task_current_save_state(struct interrupt_frame *frame)
{
    if (!task_current())
    {
        panic("No current task to save\n");
    }

    struct task *task = task_current();
    task_save_state(task, frame);
}

int task_page()
{
    user_registers();
    task_switch(current_task);
    return 0;
}

int task_page_task(struct task* task)
{
    user_registers();
    paging_switch(task_paging_desc(task));
    return 0;
}

void task_run_first_ever_task()
{
    if (!current_task)
    {
        panic("task_run_first_ever_task(): No current task exists!\n");
    }

    task_switch(task_head);
    task_return(&task_head->registers);
}

int task_init(struct task *task, struct process *process)
{
    memset(task, 0, sizeof(struct task));

    task->registers.ip = PEACHOS_PROGRAM_VIRTUAL_ADDRESS;
    if (process->filetype == PROCESS_FILETYPE_ELF)
    {
        task->registers.ip = elf_header(process->elf_file)->e_entry;
    }

    task->registers.ss = USER_DATA_SEGMENT;
    task->registers.cs = USER_CODE_SEGMENT;
    task->registers.rsp = PEACHOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;
    task->process = process;

    return 0;
}

void* task_get_stack_item(struct task* task, int index)
{
    void* result = 0;

    uint64_t* sp_ptr = (uint64_t*) task->registers.rsp;

    // Switch to the given tasks page
    task_page_task(task);

    result = (void*) sp_ptr[index];

    // Switch back to the kernel page
    kernel_page();

    return result;
}

void* task_virtual_address_to_physical(struct task* task, void* virtual_address)
{
    return paging_get_physical_address(task->process->paging_desc, virtual_address);
}