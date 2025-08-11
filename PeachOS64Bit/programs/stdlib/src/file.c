#include "file.h"
#include "peachos.h"
#include <stdint.h>
#include <stddef.h>

int fopen(const char* filename, const char* mode)
{
    return (int) peachos_fopen(filename, mode);
}

void fclose(int fd)
{
    peachos_fclose((size_t) fd);
}

int fread(void* buffer, size_t size, size_t count, long fd)
{
    return peachos_fread(buffer, size, count, fd);
}

int fseek(int fd, int offset, int whence)
{
    return (int) peachos_fseek(fd, offset, whence);
}

int fstat(int fd, struct file_stat* file_stat_out)
{
    return (int) peachos_fstat(fd, file_stat_out);
}