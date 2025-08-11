#ifndef USERLAND_FILE_H
#define USERLAND_FILE_H

#include <stdint.h>
#include <stddef.h>

int fopen(const char* filename, const char* mode);
void fclose(int fd);
int fread(void* buffer, size_t size, size_t count, long fd);
int fseek(int fd, int offset, int whence);
#endif