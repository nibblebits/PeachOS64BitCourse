#include "multiheap.h"
#include "kernel.h"
#include "memory/paging/paging.h"
#include "status.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

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

static bool multiheap_heap_allows_paging(struct multiheap_single_heap* heap)
{
    return heap->flags & MULTIHEAP_HEAP_FLAG_DEFRAGMENT_WITH_PAGING;
}

/**
 * Returns the maximum possible ending address for the heap
 */
void* multiheap_get_max_memory_end_address(struct multiheap* multiheap)
{
    void* max_addr = 0x00;
    struct multiheap_single_heap* current = multiheap->first_multiheap;
    while(current)
    {
        if(current->heap->eaddr >= max_addr)
        {
            max_addr = current->heap->eaddr;
        }
        current = current->next;
    }

    return max_addr;
}

struct multiheap_single_heap* multiheap_get_heap_for_address(struct multiheap* multiheap, void* address)
{
    struct multiheap_single_heap* current = multiheap->first_multiheap;
    while(current)
    {
        if (heap_is_address_within_heap(current->heap, address))
        {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

// COME BACK AND IMPLEMENT THE PAGING HEAP GET ADDRESS FUNCTION

bool multiheap_is_address_virtual(struct multiheap* multiheap, void* ptr)
{
    return ptr >= multiheap->max_end_data_addr;
}

void* multiheap_virtual_address_to_physical(struct multiheap* multiheap, void* ptr)
{
    void* phys_addr = (void*)((uintptr_t) ptr - ((uintptr_t) multiheap->max_end_data_addr));
    return phys_addr
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

/**
 * Called by heap.c when a block is freed, but only in paging heaps.
 */
void multiheap_paging_heap_free_block(void* ptr)
{
    paging_map(paging_current_descriptor(), ptr, NULL, 0);
}
int multiheap_ready(struct multiheap* multiheap)
{
    int res = 0;
    multiheap->flags |= MULTIHEAP_FLAG_IS_READY;

    struct paging_desc* paging_desc = paging_current_descriptor();
    if (!paging_desc)
    {
        panic("You must've had paging setup at this point for this to work\n");
    }

    void* max_end_addr = multiheap_get_max_memory_end_address(multiheap);
    multiheap->max_end_data_addr = max_end_addr;

    struct multiheap_single_heap* current = multiheap->first_multiheap;
    while(current)
    {
        if (multiheap_heap_allows_paging(current))
        {
            void *paging_heap_starting_address = max_end_addr + (uint64_t) current->heap->saddr;
            void* paging_heap_ending_address = max_end_addr + (uint64_t) current->heap->eaddr;

            struct heap_table* paging_heap_table = heap_zalloc(multiheap->starting_heap, sizeof(struct heap_table));
            paging_heap_table->entries = heap_zalloc(multiheap->starting_heap, current->heap->table->total * sizeof(HEAP_BLOCK_TABLE_ENTRY));
            paging_heap_table->total = current->heap->table->total;

            struct heap* paging_heap = heap_kalloc(multiheap->starting_heap, sizeof(struct heap));
            heap_create(paging_heap, paging_heap_starting_address, paging_heap_ending_address, paging_heap_table);

            paging_map_to(paging_current_descriptor(), paging_heap_starting_address, paging_heap_starting_address, paging_heap_ending_address, 0);

            heap_callbacks_set(paging_heap, NULL, multiheap_paging_heap_free_block);
            current->paging_heap = paging_heap;
        }
        current = current->next; 
    }

out:
    return res;
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