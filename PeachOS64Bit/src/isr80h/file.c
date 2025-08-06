#include "file.h"
#include "task/task.h"
#include "task/process.h"
#include "idt/idt.h"
#include <stddef.h>
#include <stdint.h>

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