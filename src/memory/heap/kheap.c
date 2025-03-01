#include "kheap.h"
#include "heap.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"

struct heap kernel_heap;
struct heap_table kernel_heap_table;

void kheap_init(size_t size)
{
    size_t total_table_entries = size / PEACHOS_HEAP_BLOCK_SIZE;
    kernel_heap_table.entries = (HEAP_BLOCK_TABLE_ENTRY*)(PEACHOS_HEAP_TABLE_ADDRESS);
    kernel_heap_table.total = total_table_entries;

    void* end = (void*)(PEACHOS_HEAP_ADDRESS + size);
    int res = heap_create(&kernel_heap, (void*)(PEACHOS_HEAP_ADDRESS), end, &kernel_heap_table);
    if (res < 0)
    {
        panic("Failed to create heap\n");
    }

}

struct heap* kheap_get()
{
    return &kernel_heap;
}

void* kmalloc(size_t size)
{
    void* ptr = heap_malloc(&kernel_heap, size);
    if (!ptr)
    {
        panic("Failed to allocate memory\n");
    }
    return ptr;
}

void* kzalloc(size_t size)
{
    void* ptr = kmalloc(size);
    if (!ptr)
        return 0;

    memset(ptr, 0x00, size);
    return ptr;
}

void kfree(void* ptr)
{
    heap_free(&kernel_heap, ptr);
}