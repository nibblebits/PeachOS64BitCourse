#include "process.h"
#include "config.h"
#include "status.h"
#include "task/task.h"
#include "memory/memory.h"
#include "string/string.h"
#include "fs/file.h"
#include "lib/vector/vector.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "loader/formats/elfloader.h"
#include "kernel.h"
#include <stdbool.h>

// The current process that is running
struct process *current_process = 0;

struct vector *process_vector = NULL;

int process_get_allocation_by_start_addr(struct process *process, void *addr, struct process_allocation *allocation_out);

int process_free_process(struct process *process);
int process_close_file_handles(struct process *process);

void *process_virtual_address_to_physical(struct process *process, void *virt_addr)
{
    return paging_get_physical_address(process->paging_desc, virt_addr);
}

void process_system_init()
{
    process_vector = vector_new(sizeof(struct process *), 10, 0);
}

static void process_init(struct process *process)
{
    memset(process, 0, sizeof(struct process));
    process->allocations = vector_new(sizeof(struct process_allocation), 10, 0);
    process->file_handles = vector_new(sizeof(struct process_file_handle *), 4, 0);
}

struct process *process_current()
{
    return current_process;
}

struct process *process_get(int process_id)
{
    int res = 0;
    struct process *process_out = NULL;
    res = vector_at(process_vector, process_id, &process_out, sizeof(process_out));
    if (res < 0)
    {
        return ERROR(EINVARG);
    }

    return process_out;
}

int process_switch(struct process *process)
{
    current_process = process;
    return 0;
}

int process_find_free_allocation_index(struct process *process)
{
    int res = 0;
    bool found = false;
    size_t allocation_size = vector_count(process->allocations);
    for (size_t i = 0; i < allocation_size; i++)
    {
        struct process_allocation allocation;
        res = vector_at(process->allocations, i, &allocation, sizeof(allocation));
        if (res < 0)
        {
            break;
        }

        if (allocation.ptr == NULL)
        {
            res = i;
            found = true;
            break;
        }
    }

    if (!found)
    {
        struct process_allocation allocation = {0};
        res = vector_push(process->allocations, &allocation);
    }
    return res;
}

int process_allocation_set_map(struct process *process, int allocation_entry_index, void *ptr, size_t size)
{
    int res = paging_map_to(process->paging_desc, ptr, ptr, paging_align_address(ptr + size), PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    if (res < 0)
    {
        goto out;
    }

    struct process_allocation allocation;
    res = vector_at(process->allocations, allocation_entry_index, &allocation, sizeof(allocation));
    if (res < 0)
    {
        goto out;
    }

    allocation.ptr = ptr;
    allocation.end = ptr + size;
    allocation.size = size;

    vector_overwrite(process->allocations, allocation_entry_index, &allocation, sizeof(allocation));

out:
    return res;
}

int process_allocation_exists(struct process* process, void* ptr, size_t* index_out)
{
    int res = -ENOTFOUND;
    size_t total_allocations = vector_count(process->allocations);
    for(size_t i = 0; i < total_allocations; i++)
    {
        struct process_allocation allocation;
        res = vector_at(process->allocations, i, &allocation, sizeof(allocation));
        if (res < 0)
        {
            break;
        }

        if (allocation.ptr == ptr)
        {
            if (index_out)
            {
                *index_out = i;
            }
            res = 0;
            break;
        }
    }
    return res;
}
void* process_realloc(struct process* process, void* old_virt_ptr, size_t new_size)
{
    int res = 0;
    void* new_ptr = NULL;
    void* old_phys_ptr = NULL;
    size_t old_allocation_index = 0;
    res = process_allocation_exists(process, old_virt_ptr, &old_allocation_index);
    if (res < 0)
    {
        goto out;
    }

    old_phys_ptr = old_virt_ptr;
    if (old_phys_ptr)
    {
        old_phys_ptr = task_virtual_address_to_physical(process->task, old_virt_ptr);
        if (!old_phys_ptr)
        {
            res = -ENOMEM;
            goto out;
        }
    }
    new_ptr = krealloc(old_phys_ptr, new_size);
    if (!new_ptr)
    {
        res = -ENOMEM;
        goto out;
    }

    res = process_allocation_set_map(process, old_allocation_index, new_ptr, new_size);
    if (res < 0)
    {
        goto out;
    }

out:
    return new_ptr;

}
void *process_malloc(struct process *process, size_t size)
{
    int res = 0;
    void *ptr = kzalloc(size);
    if (!ptr)
    {
        res = -ENOMEM;
        goto out_err;
    }

    int index = process_find_free_allocation_index(process);
    if (index < 0)
    {
        res = -ENOMEM;
        goto out_err;
    }

    res = process_allocation_set_map(process, index, ptr, size);
    if (res < 0)
    {
        goto out_err;
    }
    return ptr;

out_err:
    if (ptr)
    {
        kfree(ptr);
    }
    return 0;
}

static bool process_is_process_pointer(struct process *process, void *ptr)
{
    size_t total_allocations = vector_count(process->allocations);
    for (size_t i = 0; i < total_allocations; i++)
    {
        struct process_allocation allocation;
        int res = vector_at(process->allocations, i, &allocation, sizeof(allocation));
        if (res < 0)
        {
            break;
        }

        if (allocation.ptr == ptr)
        {
            return true;
        }
    }

    return false;
}

static void process_allocation_unjoin(struct process *process, void *ptr)
{
    size_t total_allocations = vector_count(process->allocations);
    for (size_t i = 0; i < total_allocations; i++)
    {
        struct process_allocation allocation;
        int res = vector_at(process->allocations, i, &allocation, sizeof(allocation));
        if (res < 0)
        {
            break;
        }

        if (allocation.ptr == ptr)
        {
            allocation.ptr = NULL;
            allocation.end = NULL;
            allocation.size = 0;
            vector_overwrite(process->allocations, i, &allocation, sizeof(allocation));
        }
    }
}

int process_get_allocation_by_start_addr(struct process *process, void *addr, struct process_allocation *allocation_out)
{
    size_t total_allocations = vector_count(process->allocations);
    for (size_t i = 0; i < total_allocations; i++)
    {
        struct process_allocation allocation;
        int res = vector_at(process->allocations, i, &allocation, sizeof(allocation));
        if (res < 0)
        {
            break;
        }
        if (allocation.ptr == addr)
        {
            *allocation_out = allocation;
            return 0;
        }
    }

    return -EIO;
}

int process_terminate_allocations(struct process *process)
{
    size_t total_allocations = vector_count(process->allocations);
    for (size_t i = 0; i < total_allocations; i++)
    {
        struct process_allocation allocation;
        int res = vector_at(process->allocations, i, &allocation, sizeof(allocation));
        if (res < 0)
        {
            break;
        }

        if (allocation.ptr)
        {
            process_free(process, allocation.ptr);
        }
    }
    return 0;
}

int process_free_binary_data(struct process *process)
{
    if (process->ptr)
    {
        kfree(process->ptr);
    }
    return 0;
}

int process_free_elf_data(struct process *process)
{
    if (process->elf_file)
    {
        elf_close(process->elf_file);
    }

    return 0;
}
int process_free_program_data(struct process *process)
{
    int res = 0;
    switch (process->filetype)
    {
    case PROCESS_FILETYPE_BINARY:
        res = process_free_binary_data(process);
        break;

    case PROCESS_FILETYPE_ELF:
        res = process_free_elf_data(process);
        break;

    default:
        res = -EINVARG;
    }
    return res;
}

void process_switch_to_any()
{
    size_t total_process_slots = vector_count(process_vector);
    for(size_t i = 0; i < total_process_slots; i++)
    {
        struct process* process = NULL;
        int res = vector_at(process_vector, i, &process, sizeof(&process));
        if (res < 0)
        {
            break;
        }

        if (process)
        {
            process_switch(process);
            return;
        }
    }
    panic("No processes to switch too\n");
}

static void process_unlink(struct process *process)
{
    struct process* null_process = NULL;
    vector_overwrite(process_vector, process->id, &null_process, sizeof(&null_process));
    if (current_process == process)
    {
        process_switch_to_any();
    }
}

int process_free_process(struct process *process)
{
    int res = 0;
    process_terminate_allocations(process);
    process_free_program_data(process);
    process_close_file_handles(process);

    // Free the process allocations
    vector_free(process->allocations);
    process->allocations = NULL;

    // Free the process stack memory.
    if (process->stack)
    {
        kfree(process->stack);
        process->stack = NULL;
    }
    // Free the task
    if (process->task)
    {
        task_free(process->task);
        process->task = NULL;
    }

    kfree(process);

out:
    return res;
}

int process_terminate(struct process *process)
{
    // Unlink the process from the process array.
    process_unlink(process);

    int res = process_free_process(process);
    if (res < 0)
    {
        goto out;
    }

out:
    return res;
}

void process_get_arguments(struct process *process, int *argc, char ***argv)
{
    *argc = process->arguments.argc;
    *argv = process->arguments.argv;
}

int process_count_command_arguments(struct command_argument *root_argument)
{
    struct command_argument *current = root_argument;
    int i = 0;
    while (current)
    {
        i++;
        current = current->next;
    }

    return i;
}

int process_inject_arguments(struct process *process, struct command_argument *root_argument)
{
    int res = 0;
    struct command_argument *current = root_argument;
    int i = 0;
    int argc = process_count_command_arguments(root_argument);
    if (argc == 0)
    {
        res = -EIO;
        goto out;
    }

    char **argv = process_malloc(process, sizeof(const char *) * argc);
    if (!argv)
    {
        res = -ENOMEM;
        goto out;
    }

    while (current)
    {
        char *argument_str = process_malloc(process, sizeof(current->argument));
        if (!argument_str)
        {
            res = -ENOMEM;
            goto out;
        }

        strncpy(argument_str, current->argument, sizeof(current->argument));
        argv[i] = argument_str;
        current = current->next;
        i++;
    }

    process->arguments.argc = argc;
    process->arguments.argv = argv;
out:
    return res;
}
void process_free(struct process *process, void *ptr)
{
    int res = 0;
    // Unlink the pages from the process for the given address
    struct process_allocation allocation;
    res = process_get_allocation_by_start_addr(process, ptr, &allocation);
    if (res < 0)
    {
        // Oops its not our pointer.
        return;
    }

    res = paging_map_to(process->paging_desc, allocation.ptr, allocation.ptr, paging_align_address(allocation.ptr + allocation.size), 0x00);
    if (res < 0)
    {
        return;
    }

    // Unjoin the allocation
    process_allocation_unjoin(process, ptr);

    // We can now free the memory.
    kfree(ptr);
}

static int process_load_binary(const char *filename, struct process *process)
{
    void *program_data_ptr = 0x00;
    int res = 0;
    int fd = fopen(filename, "r");
    if (!fd)
    {
        res = -EIO;
        goto out;
    }

    struct file_stat stat;
    res = fstat(fd, &stat);
    if (res != PEACHOS_ALL_OK)
    {
        goto out;
    }

    program_data_ptr = kzalloc(stat.filesize);
    if (!program_data_ptr)
    {
        res = -ENOMEM;
        goto out;
    }

    if (fread(program_data_ptr, stat.filesize, 1, fd) != 1)
    {
        res = -EIO;
        goto out;
    }

    process->filetype = PROCESS_FILETYPE_BINARY;
    process->ptr = program_data_ptr;
    process->size = stat.filesize;

out:
    if (res < 0)
    {
        if (program_data_ptr)
        {
            kfree(program_data_ptr);
        }
    }
    fclose(fd);
    return res;
}

static int process_load_elf(const char *filename, struct process *process)
{
    int res = 0;
    struct elf_file *elf_file = 0;
    res = elf_load(filename, &elf_file);
    if (ISERR(res))
    {
        goto out;
    }

    process->filetype = PROCESS_FILETYPE_ELF;
    process->elf_file = elf_file;
out:
    return res;
}
static int process_load_data(const char *filename, struct process *process)
{
    int res = 0;
    res = process_load_elf(filename, process);
    if (res == -EINFORMAT)
    {
        res = process_load_binary(filename, process);
    }

    return res;
}

int process_map_binary(struct process *process)
{
    int res = 0;
    paging_map_to(process->paging_desc, (void *)PEACHOS_PROGRAM_VIRTUAL_ADDRESS, process->ptr, paging_align_address(process->ptr + process->size), PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE);
    return res;
}

static int process_map_elf(struct process *process)
{
    int res = 0;

    struct elf_file *elf_file = process->elf_file;
    struct elf_header *header = elf_header(elf_file);
    struct elf64_phdr *phdrs = elf_pheader(header);
    for (int i = 0; i < header->e_phnum; i++)
    {
        struct elf64_phdr *phdr = &phdrs[i];
        void *phdr_phys_address = elf_phdr_phys_address(elf_file, phdr);
        int flags = PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL;
        if (phdr->p_flags & PF_W)
        {
            flags |= PAGING_IS_WRITEABLE;
        }
        res = paging_map_to(process->paging_desc, paging_align_to_lower_page((void *)(uintptr_t)phdr->p_vaddr), paging_align_to_lower_page(phdr_phys_address), paging_align_address(phdr_phys_address + phdr->p_memsz), flags);
        if (ISERR(res))
        {
            break;
        }
    }
    return res;
}
int process_map_memory(struct process *process)
{
    int res = 0;

    // Map all the e820 memory regions
    // so the whole address space is mapped
    paging_map_e820_memory_regions(process->paging_desc);

    switch (process->filetype)
    {
    case PROCESS_FILETYPE_ELF:
        res = process_map_elf(process);
        break;

    case PROCESS_FILETYPE_BINARY:
        res = process_map_binary(process);
        break;

    default:
        panic("process_map_memory: Invalid filetype\n");
    }

    if (res < 0)
    {
        goto out;
    }

    // Finally map the stack
    paging_map_to(process->paging_desc, (void *)PEACHOS_PROGRAM_VIRTUAL_STACK_ADDRESS_END, process->stack, paging_align_address(process->stack + PEACHOS_USER_PROGRAM_STACK_SIZE), PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE);
out:
    return res;
}

int process_get_free_slot()
{
    int res = 0;
    bool found = false;
    size_t total_process_slots = vector_count(process_vector);
    for(size_t i = 0; i < total_process_slots; i++)
    {
        struct process* process_out = NULL;
        res = vector_at(process_vector, i, &process_out, sizeof(process_out));
        if (res < 0)
        {
            break;
        }

        if (!process_out)
        {
            found = true;
            res = i;
            break;
        }
    }

    if (res < 0)
    {
        goto out;
    }

    if (!found)
    {
        struct process* null_process = NULL;
        int process_index = vector_push(process_vector, &null_process);

        // vector_push returned the index of the new null process pointer
        res = process_index;
    }
out:
    return res;
}

int process_load(const char *filename, struct process **process)
{
    int res = 0;
    int process_slot = process_get_free_slot();
    if (process_slot < 0)
    {
        res = -EISTKN;
        goto out;
    }

    res = process_load_for_slot(filename, process, process_slot);
out:
    return res;
}

int process_load_switch(const char *filename, struct process **process)
{
    int res = process_load(filename, process);
    if (res == 0)
    {
        process_switch(*process);
    }

    return res;
}

int process_load_for_slot(const char *filename, struct process **process, int process_slot)
{
    int res = 0;
    struct process *_process;

    if (process_get(process_slot) != 0)
    {
        res = -EISTKN;
        goto out;
    }

    _process = kzalloc(sizeof(struct process));
    if (!_process)
    {
        res = -ENOMEM;
        goto out;
    }

    process_init(_process);
    res = process_load_data(filename, _process);
    if (res < 0)
    {
        goto out;
    }

    _process->stack = kzalloc(PEACHOS_USER_PROGRAM_STACK_SIZE);
    if (!_process->stack)
    {
        res = -ENOMEM;
        goto out;
    }

    strncpy(_process->filename, filename, sizeof(_process->filename));
    _process->id = process_slot;

    _process->paging_desc = paging_desc_new(PAGING_MAP_LEVEL_4);
    if (!_process->paging_desc)
    {
        res = -EIO;
        goto out;
    }

    res = process_map_memory(_process);
    if (res < 0)
    {
        goto out;
    }

    // Create a task
    _process->task = task_new(_process);
    if (ERROR_I(_process->task) == 0)
    {
        res = ERROR_I(_process->task);

        // Task is NULL due to error code being returned in task_new.
        _process->task = NULL;
        goto out;
    }

    *process = _process;

    // Overwrite the free process pointer thats in the vector
    // with our allocated one. SO we take ownership of the slot.
    vector_overwrite(process_vector, process_slot, &_process, sizeof(&_process));

out:
    if (ISERR(res))
    {
        if (_process)
        {
            process_free_process(_process);
            _process = NULL;
            *process = NULL;
        }

        // Free the process data
    }
    return res;
}

bool process_is_stack_memory(struct process *process, void *addr)
{
    return (uintptr_t)addr >= PEACHOS_PROGRAM_VIRTUAL_STACK_ADDRESS_END &&
           (uintptr_t)addr <= PEACHOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;
}

int process_get_allocation_by_addr(struct process *process, void *addr, struct process_allocation_request *allocation_request_out)
{
    // Null the request
    memset(allocation_request_out, 0, sizeof(struct process_allocation_request));

    // Is this stack memory?
    if (process_is_stack_memory(process, addr))
    {
        // we have stack memory
        uint64_t addr_int = (uint64_t)addr;
        uint64_t stack_size = PEACHOS_USER_PROGRAM_STACK_SIZE;
        // START OF THE STACK IS HIGHER IN MEMORY REMEMBER
        uint64_t total_bytes_left = PEACHOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START - addr_int;
        allocation_request_out->allocation.ptr = (void *)PEACHOS_PROGRAM_VIRTUAL_STACK_ADDRESS_END;
        allocation_request_out->allocation.end = (void *)PEACHOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;
        allocation_request_out->allocation.size = stack_size;
        allocation_request_out->flags |= PROCESS_ALLOCATION_REQUEST_IS_STACK_MEMORY;
        allocation_request_out->peek.addr = addr;
        allocation_request_out->peek.end = (void *)PEACHOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;
        allocation_request_out->peek.total_bytes_left = total_bytes_left;
        return 0;
    }

    // Not a stack address then check the heap
    size_t total_allocations = vector_count(process->allocations);
    for (size_t i = 0; i < total_allocations; i++)
    {
        struct process_allocation allocation;
        int res = vector_at(process->allocations, i, &allocation, sizeof(allocation));
        if (res < 0)
        {
            break;
        }

        uint64_t allocation_addr = (uint64_t)allocation.ptr;
        uint64_t allocation_addr_end = (uint64_t)allocation.end;
        if ((uint64_t)addr >= allocation_addr &&
            ((uint64_t)addr) <= allocation_addr_end)
        {
            size_t bytes_used = (uint64_t)addr - allocation_addr;
            size_t bytes_left = allocation_addr_end - bytes_used;
            allocation_request_out->allocation = allocation;
            allocation_request_out->peek.addr = addr;
            allocation_request_out->peek.end = (void *)allocation_addr_end;
            allocation_request_out->peek.total_bytes_left = bytes_left;
            return 0;
        }
    }

    return -EIO;
}

int process_validate_memory_or_terminate(struct process *process, void *virt_addr, size_t space_needed)
{
    int res = 0;
    struct process_allocation_request allocation_request;
    res = process_get_allocation_by_addr(process, virt_addr, &allocation_request);
    if (res < 0)
    {
        goto out;
    }

    if (allocation_request.peek.total_bytes_left < space_needed)
    {
        res = -EINVARG;
        goto out;
    }
out:
    if (res < 0)
    {
        process_terminate(process);
    }
    return res;
}
int process_fstat(struct process *process, int fd, struct file_stat *virt_filestat_addr)
{
    int res = 0;
    res = process_validate_memory_or_terminate(process, virt_filestat_addr, sizeof(*virt_filestat_addr));
    if (res < 0)
    {
        goto out;
    }

    struct file_stat *phys_filestat_addr = process_virtual_address_to_physical(process, virt_filestat_addr);
    if (!phys_filestat_addr)
    {
        res = -EINVARG;
        goto out;
    }

    res = fstat(fd, phys_filestat_addr);
    if (res < 0)
    {
        goto out;
    }

out:
    return res;
}
int process_fseek(struct process *process, int fd, int offset, FILE_SEEK_MODE whence)
{
    int res = 0;
    struct process_file_handle *handle = process_file_handle_get(process, fd);
    if (!handle)
    {
        res = -EIO;
        goto out;
    }

    res = fseek(handle->fd, offset, whence);
out:
    return res;
}
int process_fread(struct process *process, void *virt_ptr, uint64_t size, uint64_t nmemb, int fd)
{
    int res = 0;

    struct process_file_handle *handle = process_file_handle_get(process, fd);
    if (!handle)
    {
        res = -EIO;
        goto out;
    }

    size_t true_size = size * nmemb;
    res = process_validate_memory_or_terminate(process, virt_ptr, true_size);
    if (res < 0)
    {
        goto out;
    }

    void *phys_ptr = task_virtual_address_to_physical(process->task, virt_ptr);
    if (!phys_ptr)
    {
        goto out;
    }

    res = fread(phys_ptr, size, nmemb, handle->fd);
    if (res < 0)
    {
        goto out;
    }

out:
    return res;
}
int process_fclose(struct process *process, int fd)
{
    int res = 0;
    struct process_file_handle *handle = process_file_handle_get(process, fd);
    if (!handle)
    {
        return -EINVARG;
    }

    fclose(handle->fd);
    vector_pop_element(process->file_handles, &handle, sizeof(handle));
    kfree(handle);
    return res;
}

int process_fopen(struct process *process, const char *path, const char *mode)
{
    int res = 0;
    int fd = fopen(path, mode);
    if (fd <= 0)
    {
        res = -EIO;
        goto out;
    }

    res = fd;
    // Allocate memory for the file handle
    struct process_file_handle *handle = kzalloc(sizeof(struct process_file_handler *));
    if (!handle)
    {
        res = -ENOMEM;
        goto out;
    }

    handle->fd = fd;
    strncpy(handle->file_path, path, sizeof(handle->file_path));
    strncpy(handle->mode, mode, sizeof(handle->mode));

    // Push to the file handles vector so we have a record
    vector_push(process->file_handles, &handle);
out:
    if (res < 0)
    {
        fclose(fd);
    }
    return res;
}

struct process_file_handle *process_file_handle_get(struct process *process, int fd)
{
    size_t total_handles = vector_count(process->file_handles);
    for (size_t i = 0; i < total_handles; i++)
    {
        struct process_file_handle *handle = NULL;
        vector_at(process->file_handles, i, &handle, sizeof(handle));
        if (handle && handle->fd == fd)
        {
            return handle;
        }
    }

    return NULL;
}

int process_close_file_handles(struct process *process)
{
    int res = 0;
    size_t total_handles = vector_count(process->file_handles);
    for (size_t i = 0; i < total_handles; i++)
    {
        struct process_file_handle *handle = NULL;
        vector_at(process->file_handles, i, &handle, sizeof(handle));
        if (handle)
        {
            fclose(handle->fd);
            kfree(handle);
        }
    }

    vector_free(process->file_handles);
    process->file_handles = NULL;
    return res;
}