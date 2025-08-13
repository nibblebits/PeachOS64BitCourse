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