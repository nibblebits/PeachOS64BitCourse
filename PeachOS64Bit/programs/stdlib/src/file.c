#include "file.h"
#include "peachos.h"

int fopen(const char* filename, const char* mode)
{
    return (int) peachos_fopen(filename, mode);
}