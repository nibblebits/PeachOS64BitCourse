#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stddef.h>

void kheap_init();
void* kmalloc(size_t size);
void* kzalloc(size_t size);
void* kpalloc(size_t size);
void* kpzalloc(size_t size);
void kfree(void* ptr);
struct heap* kheap_get();

void kheap_post_paging();

#endif