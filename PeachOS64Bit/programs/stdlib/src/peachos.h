#ifndef PEACHOS_H
#define PEACHOS_H
#include <stddef.h>
#include <stdbool.h>


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

// Forward declare file stat.
struct file_stat;

void print(const char* filename);
int peachos_getkey();

void* peachos_malloc(size_t size);
void peachos_free(void* ptr);
void peachos_putchar(char c);
int peachos_getkeyblock();
void peachos_terminal_readline(char* out, int max, bool output_while_typing);
void peachos_process_load_start(const char* filename);
struct command_argument* peachos_parse_command(const char* command, int max);
void peachos_process_get_arguments(struct process_arguments* arguments);
int peachos_system(struct command_argument* arguments);
int peachos_system_run(const char* command);
void peachos_exit();

int peachos_fopen(const char* filename, const char* mode);
void peachos_fclose(size_t fd);
long peachos_fread(void* buffer, size_t size, size_t count, long fd);
long peachos_fseek(long fd, long offset, long whence);
long peachos_fstat(long fd, struct file_stat* file_stat_out);
void* peachos_realloc(void* old_ptr, size_t new_size);
#endif