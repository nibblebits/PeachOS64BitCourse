#include "multiheap.h"
#include "kernel.h"
#include "status.h"

struct multiheap *multiheap_new(struct heap *starting_heap)
{
    struct multiheap *multiheap = heap_zalloc(starting_heap, sizeof(struct multiheap));
    if (!multiheap)
    {
        goto out;
    }
    multiheap->starting_heap = starting_heap;
    multiheap->first_multiheap = 0;
    multiheap->total_heaps = 0;
out:
    return multiheap;
}

struct multiheap_single_heap* multiheap_get_last_heap(struct multiheap* multiheap)
{
    struct multiheap_single_heap* current = multiheap->first_multiheap;
    while(current->next != 0)
    {
        current = current->next;
    }
    return current;
}
int multiheap_add_heap(struct multiheap* multiheap, struct heap* heap, int flags)
{
    struct multiheap_single_heap* new_heap = heap_zalloc(multiheap->starting_heap, sizeof(struct multiheap_single_heap));
    if (!new_heap)
    {
        return -ENOMEM;
    }

    new_heap->heap = heap;
    new_heap->next = 0;
    new_heap->flags = flags;
    if (multiheap->first_multiheap == 0)
    {
        multiheap->first_multiheap = new_heap;
    }
    else
    {
        struct multiheap_single_heap* last = multiheap_get_last_heap(multiheap);
        last->next = new_heap;
    }

    multiheap->total_heaps += 1;
    return 0;
}

int multiheap_add_existing_heap(struct multiheap* multiheap, struct heap* heap, int flags)
{
    flags |= MULTIHEAP_HEAP_FLAG_EXTERNALLY_OWNED;
    return multiheap_add_heap(multiheap, heap, flags);
}

int multiheap_add(struct multiheap *multiheap, void *saddr, void *eaddr, int flags)
{
    struct heap *heap = heap_zalloc(multiheap->starting_heap, sizeof(struct heap));
    struct heap_table *table = heap_zalloc(multiheap->starting_heap, sizeof(struct heap_table));
    if (!heap || !table)
    {
        return -ENOMEM;
    }

    int res = heap_create(heap, saddr, eaddr, table);
    if (res < 0)
    {
        heap_free(multiheap->starting_heap, heap);
        heap_free(multiheap->starting_heap, table);
        return res;
    }

    return multiheap_add_heap(multiheap, heap, flags);
}

void multiheap_free(struct multiheap* multiheap)
{
    struct multiheap_single_heap* current = multiheap->first_multiheap;
    while(current != 0)
    {
        struct multiheap_single_heap* next = current->next;
        if (!(current->flags & MULTIHEAP_HEAP_FLAG_EXTERNALLY_OWNED))
        {
            heap_free(multiheap->starting_heap, current->heap);
        }
        current = next;
    }

    heap_free(multiheap->starting_heap, multiheap);
}

void* multiheap_alloc_first_pass(struct multiheap* multiheap, size_t size)
{
    void* allocation_ptr = NULL;
    struct multiheap_single_heap* current = multiheap->first_multiheap;
    while(current != 0)
    {
        allocation_ptr = heap_malloc(current->heap, size);
        if (allocation_ptr)
        {
            // Memory was allocated
            break;
        }

        current = current->next;
    }

    return allocation_ptr;
}

void* multiheap_alloc_second_pass(struct multiheap* multiheap, size_t size)
{
    // TO IMPLEMENT, DEFRAGMENT AND FIND ENOUGH BLOCKS OR FAIL RETURN NULL.
    return NULL;
}

void* multiheap_alloc(struct multiheap* multiheap, size_t size)
{
    void* allocation_ptr = multiheap_alloc_first_pass(multiheap, size);
    if (allocation_ptr)
    {
        return allocation_ptr;
    }

    // Normal alloc does not defragment with paging
    return NULL;
}

void* multiheap_palloc(struct multiheap* multiheap, size_t size)
{
    void* allocation_ptr = multiheap_alloc_first_pass(multiheap, size);
    if (allocation_ptr)
    {
        return allocation_ptr;
    }

    // Possible fragmentation, no pointer able to be found
    // in all heaps.
    // perform second pass..

    allocation_ptr = multiheap_alloc_second_pass(multiheap, size);
    return allocation_ptr;
}