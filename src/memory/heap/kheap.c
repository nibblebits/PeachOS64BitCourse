#include "kheap.h"
#include "heap.h"
#include "multiheap.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"
// paaging
#include "memory/paging/paging.h"

// Minimal memory allocator for the kernel
// Just enough memory for us to setup a more advanced heap
struct heap kernel_minmal_heap;
struct heap_table kernel_minimal_heap_table;

struct multiheap *kernel_multiheap = NULL;

// get the first available memory region for a minmal heap
struct e820_entry *kheap_get_allowable_memory_region_for_minimal_heap()
{
    struct e820_entry *entry = 0;
    size_t total_entries = e820_total_entries();
    for (size_t i = 0; i < total_entries; i++)
    {
        struct e820_entry *current = e820_entry(i);
        // Must be available and a minimum of 100 MB of memory
        if (current->type == 1 && current->length >= PEACHOS_HEAP_MINIMUM_SIZE_BYTES)
        {
            entry = current;
            break;
        }
    }

    return entry;
}

void kheap_init()
{

    /**
     * We need to first setup a minimal heap that uses static addresses
     * as we do not have any heap yet to allocate memory. Once we have our minimal heap we can setup a multiheap
     * which will allow us to create multiple heaps within the kernel using dynamicc memory.
     */
    // lets get the first available memory region
    struct e820_entry *entry = kheap_get_allowable_memory_region_for_minimal_heap();
    if (!entry)
    {
        panic("Theres no available memory region thats a minimum of 100 MB to be used for the minimal heap\n");
    }

    // Get the address
    void *address = (void *)entry->base_addr;
    void *end_address = (void *)(entry->base_addr + entry->length);
    void *heap_table_address = address;
    // if the address is below our kernel minimal heap table address then we need to make it our heap table address address
    if (heap_table_address < (void *)PEACHOS_MINIMAL_HEAP_TABLE_ADDRESS)
    {
        // Static address, but it will be fine as we know we have 100 MB of memory
        heap_table_address = (void *)PEACHOS_MINIMAL_HEAP_TABLE_ADDRESS;
    }

    size_t total_heap_size = end_address - heap_table_address;
    size_t total_heap_blocks = total_heap_size / PEACHOS_HEAP_BLOCK_SIZE;
    size_t total_heap_entry_table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * total_heap_blocks;

    // Minimal heap is directly after the heap table
    void *heap_address = heap_table_address + total_heap_entry_table_size;

    // End address where the heap ends, also the address where new heap blocks can be created
    void *heap_end_address = end_address;

    // if the heap address isnt aligned to the block size then we need to align it
    if (!paging_is_aligned(heap_address))
    {
        heap_address = paging_align_address(heap_address);
    }

    // if the heap end address isnt aligned to the block size then we need to align it
    if (!paging_is_aligned(heap_end_address))
    {
        // will align to the lower page as we don twant to grant more memory tha nthere is
        heap_end_address = paging_align_to_lower_page(heap_end_address);
    }

    // How much data do we have to allocate for our minimal heap
    size_t size = heap_end_address - heap_address;

    size_t total_table_entries = size / PEACHOS_HEAP_BLOCK_SIZE;
    kernel_minimal_heap_table.entries = (HEAP_BLOCK_TABLE_ENTRY *)(heap_table_address);
    kernel_minimal_heap_table.total = total_table_entries;

    int res = heap_create(&kernel_minmal_heap, heap_address, heap_end_address, &kernel_minimal_heap_table);
    if (res < 0)
    {
        panic("Failed to create heap\n");
    }

    // We got our heap lets now create a multiheap, our multiheap will use the memory of the minimal heap
    kernel_multiheap = multiheap_new(&kernel_minmal_heap);
    // We don't want to waste space so why not allow our minimal heap to also be used as a heap
    multiheap_add_existing_heap(kernel_multiheap, &kernel_minmal_heap, MULTIHEAP_HEAP_FLAG_DEFRAGMENT_WITH_PAGING);

    // We want to keep track of the entry we just used so we dont add it again.
    struct e820_entry *used_entry = entry;

    // Lets now add all other available memory regions to our multiheap
    size_t total_entries = e820_total_entries();
    for (size_t i = 0; i < total_entries; i++)
    {
        struct e820_entry *current = e820_entry(i);
        if (current != used_entry && current->type == 1)
        {
            void *base_addr = (void *)current->base_addr;
            void *end_addr = (void *)(current->base_addr + current->length);

            if (base_addr < (void *)PEACHOS_MINIMAL_HEAP_TABLE_ADDRESS)
            {
                // Static address, but it will be fine as we know we have 100 MB of memory
                base_addr = (void *)PEACHOS_MINIMAL_HEAP_TABLE_ADDRESS;
            }

            if (end_addr <= base_addr)
            {
                // We cant use this one as the changes we made to the base address made it so that the end address is less than the base address
                continue;
            }

            // lets enforce alignment
            if (!paging_is_aligned(base_addr))
            {
                base_addr = paging_align_address(base_addr);
            }

            if (!paging_is_aligned(end_addr))
            {
                end_addr = paging_align_to_lower_page(end_addr);
            }

            // lets add it to our multiheap
            multiheap_add(kernel_multiheap, base_addr, end_addr, MULTIHEAP_HEAP_FLAG_DEFRAGMENT_WITH_PAGING);
        }
    }
}

/**
 * CALL ONCE PAGING IS SETUP!!!!
 */
void kheap_post_paging()
{
    // We are ready only when paging is ready.
    multiheap_ready(kernel_multiheap); // TODO THINK OF A BETTER NAME for this function...
}

void *kmalloc(size_t size)
{
    void *ptr = multiheap_alloc(kernel_multiheap, size);
    return ptr;
}

void *kzalloc(size_t size)
{
    void *ptr = kmalloc(size);
    if (!ptr)
        return 0;

    memset(ptr, 0x00, size);
    return ptr;
}

/**
 * Memory allocation that if fails will result in using paging to solve the fragmentation problem.
 * If still fails returns NULL.
 */
void *kpalloc(size_t size)
{
    void *ptr = multiheap_palloc(kernel_multiheap, size);
    return ptr;
}

void *kpzalloc(size_t size)
{
    void *ptr = kpalloc(size);
    if (!ptr)
        return 0;

    memset(ptr, 0x00, size);
    return ptr;
}

void kfree(void *ptr)
{
    multiheap_free(kernel_multiheap, ptr);
}