#include "kheap.h"
#include "heap.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
#include "multiheap.h"

struct heap kernel_minimal_heap;
struct heap_table kernel_minimal_heap_table;

struct multiheap* kernel_multiheap = NULL;

struct e820_entry* kheap_get_allowable_memory_region_for_minimal_heap()
{
    struct e820_entry* entry = 0;
    size_t total_entries = e820_total_entries();
    for(size_t i = 0; i < total_entries; i++)
    {
        struct e820_entry* current = e820_entry(i);
        if (current->type == 1 && current->length > PEACHOS_HEAP_SIZE_BYTES)
        {
            entry  = current;
            break;
        }
    }

    return entry;
}

void kheap_init()
{
    struct e820_entry* entry = kheap_get_allowable_memory_region_for_minimal_heap();
    if (!entry)
    {
        panic("Installed RAM does not meet the requirements for multiheap\n");
    }

    void* address = (void*) entry->base_addr;
    void* heap_table_address = address;
    if (heap_table_address < (void*) PEACHOS_MINIMAL_HEAP_TABLE_ADDRESS)
    {
        heap_table_address = (void*) PEACHOS_MINIMAL_HEAP_TABLE_ADDRESS;
    }

    void* heap_address = heap_table_address + PEACHOS_MINIMAL_HEAP_TABLE_SIZE;
    void* heap_end_address =  heap_address + PEACHOS_HEAP_SIZE_BYTES;

    // Check if the heap address is aligned
    if (!paging_is_aligned(heap_address))
    {
        // Not aligned align it
        heap_address = paging_align_address(heap_address);
    }

    // Check if the heap end address is aligned
    if (!paging_is_aligned(heap_end_address))
    {
        heap_end_address = paging_align_to_lower_page(heap_end_address);
    }

    size_t size = heap_end_address - heap_address;
    size_t total_table_entries = size / PEACHOS_HEAP_BLOCK_SIZE;
    kernel_minimal_heap_table.entries = (HEAP_BLOCK_TABLE_ENTRY*)(heap_table_address);
    kernel_minimal_heap_table.total = total_table_entries;

    int res = heap_create(&kernel_minimal_heap, heap_address, heap_end_address, &kernel_minimal_heap_table);
    if (res < 0)
    {
        panic("Failed to initialize minimal heap\n");
    }

    kernel_multiheap = multiheap_new(&kernel_minimal_heap);
    multiheap_add_existing_heap(kernel_multiheap, &kernel_minimal_heap, MULTIHEAP_HEAP_FLAG_EXTERNALLY_OWNED);

    struct e820_entry* used_entry = entry;

    size_t total_entries = e820_total_entries();
    for(size_t i = 0; i < total_entries; i++)
    {
        struct e820_entry* current = e820_entry(i);
        if (current != used_entry && current->type == 1)
        {
            void* base_addr = (void*) current->base_addr;
            void* end_addr = (void*) (current->base_addr + current->length);

            // Enforce page alignment
            if(!paging_is_aligned(base_addr))
            {
                base_addr = paging_align_address(base_addr);
            }

            if (!paging_is_aligned(end_addr))
            {
                end_addr = paging_align_to_lower_page(end_addr);
            }

            // Add the memory region to the multiheap
            multiheap_add(kernel_multiheap, (void*) base_addr, (void*) end_addr, MULTIHEAP_HEAP_FLAG_DEFRAGMENT_WITH_PAGING);
        }
    }
}

void* kmalloc(size_t size)
{
    void* ptr = multiheap_alloc(kernel_multiheap, size);
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

void* kpalloc(size_t size)
{
    void* ptr = multiheap_palloc(kernel_multiheap, size);
    if (!ptr)
    {
        panic("Failed to allocate memory\n");
    }
    return ptr;
}

void* kpzalloc(size_t size)
{
    void* ptr = kpalloc(size);
    if (!ptr)
        return 0;

    memset(ptr, 0x00, size);
    return ptr;
}



void kfree(void* ptr)
{
    //heap_free(&kernel_heap, ptr);
}