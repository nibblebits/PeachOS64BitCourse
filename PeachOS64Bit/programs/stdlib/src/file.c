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