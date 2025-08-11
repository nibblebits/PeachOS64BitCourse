#ifndef PROCESS_H
#define PROCESS_H
#include <stdint.h>
#include <stdbool.h>

#include "task.h"
#include "fs/file.h"
#include "config.h"

#define PROCESS_FILETYPE_ELF 0
#define PROCESS_FILETYPE_BINARY 1

typedef unsigned char PROCESS_FILETYPE;

struct process_allocation
{
    void* ptr;
    void* end;
    size_t size;
};

enum
{
    PROCESS_ALLOCATION_REQUEST_IS_STACK_MEMORY = 0b00000001,
};

struct process_allocation_request
{
    struct process_allocation allocation;
    int flags;
    struct
    {
        void* addr;
        void* end;

        size_t total_bytes_left;
    } peek;
};

struct command_argument
{
    char argument[512];
    struct command_argument* next;
};

struct process_arguments
{
    int argc;
    char** argv;
};


struct process_file_handle
{
    // File number returend by fopen
    int fd;

    // filepath
    char file_path[PEACHOS_MAX_PATH];

    // Mode, "w", "r", "w+"
    char mode[2];
};

struct process
{
    // The process id
    uint16_t id;

    char filename[PEACHOS_MAX_PATH];

    // The main process task
    struct task* task;

    // The memory (malloc) allocations of the process
    struct vector* allocations;
    
    // File handle vector,
    // vector of struct process_file_handle*
    struct vector* file_handles;

    PROCESS_FILETYPE filetype;

    union
    {
        // The physical pointer to the process memory.
        void* ptr;
        struct elf_file* elf_file;
    };
    

    // The physical pointer to the stack memory
    void* stack;

    // The size of the data pointed to by "ptr"
    uint32_t size;

    struct keyboard_buffer
    {
        char buffer[PEACHOS_KEYBOARD_BUFFER_SIZE];
        int tail;
        int head;
    } keyboard;


    // The arguments of the process.
    struct process_arguments arguments;
};

int process_switch(struct process* process);
int process_load_switch(const char* filename, struct process** process);
int process_load(const char* filename, struct process** process);
int process_load_for_slot(const char* filename, struct process** process, int process_slot);
struct process* process_current();
struct process* process_get(int process_id);
void* process_malloc(struct process* process, size_t size);
void process_free(struct process* process, void* ptr);

void process_get_arguments(struct process* process, int* argc, char*** argv);
int process_inject_arguments(struct process* process, struct command_argument* root_argument);
int process_terminate(struct process* process);

struct process_file_handle* process_file_handle_get(struct process* process, int fd);
int process_fopen(struct process* process, const char* path, const char* mode);
int process_fclose(struct process* process, int fd);
int process_fread(struct process* process, void* virt_ptr, uint64_t size, uint64_t nmemb, int fd);
int process_fseek(struct process* process, int fd, int offset, FILE_SEEK_MODE whence);

#endif