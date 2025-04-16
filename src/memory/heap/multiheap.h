#ifndef KERNEL_MULTIHEAP_H
#define KERNEL_MULTIHEAP_H

#include "heap.h"
enum
{
    // Set if the heap was created externally and its memory should not
    // be managed by our multiheap
    MULTIHEAP_HEAP_FLAG_EXTERNALLY_OWNED = 0b00000001,
    MULTIHEAP_HEAP_FLAG_DEFRAGMENT_WITH_PAGING = 0b00000010
};

struct multiheap_single_heap
{
    struct heap* heap;
    struct heap* paging_heap;
    int flags;
    struct multiheap_single_heap* next;
};

enum
{
    MULTIHEAP_FLAG_IS_READY = 0x01
};

struct multiheap
{
    // This heap is used to allocate space for the multiheap.
    struct heap* starting_heap;

    // The linked list for the first heap
    struct multiheap_single_heap* first_multiheap;

    void* max_end_data_addr;
    int flags;
    size_t total_heaps;
};

int multiheap_add_existing_heap(struct multiheap* multiheap, struct heap* heap, int flags);
int multiheap_add(struct multiheap* multiheap, void* saddr, void* eaddr, int flags);
void* multiheap_alloc(struct multiheap* multiheap, size_t size);
void* multiheap_palloc(struct multiheap* multiheap, size_t size);
struct multiheap* multiheap_new(struct heap* starting_heap);
void multiheap_free(struct multiheap* multiheap, void* ptr);

#endif