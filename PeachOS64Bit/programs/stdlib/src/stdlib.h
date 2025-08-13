#ifndef PEACHOS_STDLIB_H
#define PEACHOS_STDLIB_H
#include <stddef.h>

void* calloc(size_t n_memb, size_t size);
void* realloc(void* ptr, size_t new_size);
void* malloc(size_t size);
void free(void* ptr);
char* itoa(int i);
#endif