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

int multiheap_ready(struct multiheap* multiheap);
size_t multiheap_allocation_byte_count(struct multiheap* multiheap, void* ptr);
size_t multiheap_allocation_block_count(struct multiheap* multiheap, void* ptr);
bool multiheap_can_add_heap(struct multiheap* multiheap);
bool multiheap_is_ready(struct multiheap* multiheap);
bool multiheap_is_address_virtual(struct multiheap* multiheap, void* ptr);
struct multiheap_single_heap* multiheap_get_heap_for_address(struct multiheap* multiheap, void* address);
static bool multiheap_heap_allows_paging(struct multiheap_single_heap* heap);

int multiheap_add_existing_heap(struct multiheap* multiheap, struct heap* heap, int flags);
int multiheap_add(struct multiheap* multiheap, void* saddr, void* eaddr, int flags);
void* multiheap_alloc(struct multiheap* multiheap, size_t size);
void* multiheap_palloc(struct multiheap* multiheap, size_t size);
struct multiheap* multiheap_new(struct heap* starting_heap);
void multiheap_free(struct multiheap* multiheap, void* ptr);
void multiheap_free_heap(struct multiheap* multiheap);
void* multiheap_realloc(struct multiheap* multiheap, void* old_ptr, size_t new_size);

#endif