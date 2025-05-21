#include "multiheap.h"
#include "kernel.h"
#include "status.h"
#include "memory/paging/paging.h"

struct multiheap *multiheap_new(struct heap *starting_heap)
{
    struct multiheap *multiheap = heap_zalloc(starting_heap, sizeof(struct multiheap));
    multiheap->starting_heap = starting_heap;
    multiheap->first_multiheap = 0;
    multiheap->total_heaps = 0;
    return multiheap;
}

// get last heap
struct multiheap_single_heap *multiheap_get_last_heap(struct multiheap *multiheap)
{
    struct multiheap_single_heap *current = multiheap->first_multiheap;
    while (current->next != 0)
    {
        current = current->next;
    }

    return current;
}

static bool multiheap_heap_allows_paging(struct multiheap_single_heap *heap)
{
    return heap->flags & MULTIHEAP_HEAP_FLAG_DEFRAGMENT_WITH_PAGING;
}

/**
 * Returns the maximum possible ending address for the heap.
 *
 * For example if you have two heaps allocated forr addresses
 * 0x0000000 to 0x10000000
 *
 * and then you have from 0x10000000 to 0x20050000
 *
 * then 0x20050000 would be returned here because this is the max real address in memory we can access that
 * actually exists in RAM.
 */
void *multiheap_get_max_memory_end_address(struct multiheap *multiheap)
{
    void *max_addr = 0x00;

    struct multiheap_single_heap *current = multiheap->first_multiheap;
    while (current)
    {
        if (current->heap->eaddr >= max_addr)
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
        if(heap_is_address_within_heap(current->heap, address))
        {
            return current;
        }
        current = current->next;
    }

    return 0;
}


struct multiheap_single_heap* multiheap_get_paging_heap_for_address(struct multiheap* multiheap, void* address)
{
    struct multiheap_single_heap* current = multiheap->first_multiheap;
    while(current)
    {
        // This heap doesnt allow paging?
        if (!multiheap_heap_allows_paging(current))
        {
            continue;
        }

        if(heap_is_address_within_heap(current->paging_heap, address))
        {
            return current;
        }
        current = current->next;
    }

    return 0;
}



bool multiheap_is_address_virtual(struct multiheap* multiheap, void* ptr)
{
    // Is this a paging address or not? Well it will be if its above the max end address
    // as that means we must be using virtual memory
    return ptr >= multiheap->max_end_data_addr;
}


void* multiheap_virtual_address_to_physical(struct multiheap* multiheap, void* ptr)
{
    // We know that the addresses essentially mirror 
    // the real heap, so we can just subtract the data end address to find the real pointer
    // but a more accurate measure would be to extract from the page tables

    void* phys_addr = (void*)((uintptr_t) ptr - ((uintptr_t)multiheap->max_end_data_addr));
    return phys_addr;
}

void multiheap_get_heap_and_paging_heap_for_address(struct multiheap *multiheap, void *ptr, struct multiheap_single_heap **heap_out, struct multiheap_single_heap **paging_heap_out, void** real_phys_addr)
{
    void* real_addr = ptr;
    // Is this a paging address or not?
    if (multiheap_is_address_virtual(multiheap, ptr))
    {
        // THis is a paging fragmented address, lets get the heap that its associated with
        *paging_heap_out = multiheap_get_paging_heap_for_address(multiheap, ptr);

        // Recalculate the real addres
        real_addr = multiheap_virtual_address_to_physical(multiheap, ptr);
    }

    *heap_out = multiheap_get_heap_for_address(multiheap, real_addr);
    *real_phys_addr = real_addr;
}




/**
 * Called as an event handler in the paging_heap, gets called when theirs a free of a block within
 * the paging_heap. We will be responsible for unmapping that memory now.
 */
void multiheap_paging_heap_free_block(void* ptr)
{
    // We will map the address to NULL so it page faults if accessed again.
    paging_map(paging_current_descriptor(), ptr, NULL, 0);
}

// Think of a better function name TODO:
/**
 * To be called when you have finished adding heaps to the multiheap
 */
int multiheap_ready(struct multiheap *multiheap)
{
    int res = 0;

    multiheap->flags |= MULTIHEAP_FLAG_IS_READY;

    // Get the current paging descriptor
    struct paging_desc *paging_desc = paging_current_descriptor();
    if (!paging_desc)
    {
        panic("You must've had paging setup at this point for this to work..");
    }

    // We use only the current descriptor
    // this may be a problem in the future if you switch descriptors without remapping the memory
    // ill consider this later..

    // Get the max end address so we know where we can start to declare
    // our virtual memory (non RAM addresses) that will be used to solve defragmentation in the heap
    void *max_end_addr = multiheap_get_max_memory_end_address(multiheap);
    multiheap->max_end_data_addr = max_end_addr;

    // Let's loop through all the entires and any that allow paging we must create a paging heap for them
    struct multiheap_single_heap *current = multiheap->first_multiheap;
    while (current)
    {
        if (multiheap_heap_allows_paging(current))
        {
            // We are allowed to use paging to resolve defragmentation.
            // Great.. Lets create a paging heap for this heap.
            // but we will need to know the virtual addresses to use, to do that we must know max memory first.
            // max memory is in the max_end_addr.
            // How we will do this, we will mirror the real heap address, adding the heap address
            // to the max end address to calculate the final virtual address
            // This will act as a mirror from the real heap to the fake heap, allowing us to use the fake heap
            // for defragmentation purposes later.

            void *paging_heap_starting_address = max_end_addr + (uint64_t)current->heap->saddr;
            void *paging_heap_ending_address = max_end_addr + (uint64_t)current->heap->eaddr;

            // Now we have the mirror  lets create that paging heap table

            // Ideally this functionality would be best suited in heap.c but the problem is
            // we aren't guaranteed to have a heap at that time to declare memory too
            // this is why manual action is needed when setting up the heap tables.
            struct heap_table *paging_heap_table = heap_zalloc(multiheap->starting_heap, sizeof(struct heap_table));
            paging_heap_table->entries = heap_zalloc(multiheap->starting_heap, current->heap->table->total * sizeof(HEAP_BLOCK_TABLE_ENTRY));
            paging_heap_table->total = current->heap->table->total;

            // Ok now finally the paging heap can be made

            struct heap *paging_heap = heap_zalloc(multiheap->starting_heap, sizeof(struct heap));
            heap_create(paging_heap, paging_heap_starting_address, paging_heap_ending_address, paging_heap_table);

            // We should map the starting and end addresses just so a page table entry is created
            // for them, incase we run out of memory later and are not able to do it
            // they will just point to NULL and they arent present.
            // THe mapping has just one purpose and that is to ensure all the required
            // memory to deal with these regions is allocated.
            // we really dont care about those page tables at this point and they aren't present
            // and cannot be accessed.
            paging_map_to(paging_current_descriptor(), paging_heap_starting_address, paging_heap_starting_address, paging_heap_ending_address, 0);

            // WARNING: Now that the paging heap is created its very important to never read or write
            // to that memory, it exists purley to keep track of the paging addresses, it is not real RAM
            // it is used to resolve fragmentation problems in the true heap.
            // Allocations are fine, just do not write or zero the memory, or read!
            // The paging heaps act as a mirror to the genuine heap, a mirror of entries
            // that are only used when fragmentation occurs.

            // Let's setup the calbacks for the paging heap
            heap_callbacks_set(paging_heap, NULL, multiheap_paging_heap_free_block);

            // Set the paging heap 
            current->paging_heap = paging_heap;
        }
        current = current->next;
    }

out:
    return res;
}

bool multiheap_is_ready(struct multiheap *multiheap)
{
    return multiheap->flags & MULTIHEAP_FLAG_IS_READY;
}

bool multiheap_can_add_heap(struct multiheap *multiheap)
{
    return !multiheap_is_ready(multiheap);
}

int multiheap_add_heap(struct multiheap *multiheap, struct heap *heap, int flags)
{
    if (!multiheap_can_add_heap(multiheap))
    {
        // We aren't allowed to add heaps at this stage.
        return -EINVARG;
    }

    struct multiheap_single_heap *new_heap = heap_zalloc(multiheap->starting_heap, sizeof(struct multiheap_single_heap));
    new_heap->heap = heap;
    new_heap->next = 0;
    new_heap->flags = flags;
    if (multiheap->first_multiheap == 0)
    {
        multiheap->first_multiheap = new_heap;
    }
    else
    {
        struct multiheap_single_heap *last = multiheap_get_last_heap(multiheap);
        last->next = new_heap;
    }

    multiheap->total_heaps += 1;
    return 0;
}

int multiheap_add_existing_heap(struct multiheap *multiheap, struct heap *heap, int flags)
{
    flags |= MULTIHEAP_HEAP_FLAG_EXTERNALLY_OWNED;
    return multiheap_add_heap(multiheap, heap, flags);
}

int multiheap_add(struct multiheap *multiheap, void *saddr, void *eaddr, int flags)
{
    int res = 0;
    struct heap *heap = heap_zalloc(multiheap->starting_heap, sizeof(struct heap));
    struct heap_table *table = heap_zalloc(multiheap->starting_heap, sizeof(struct heap_table));
    if (!heap || !table)
    {
        res = -ENOMEM;
        goto out;
    }

    // Create the heap
    res = heap_create(heap, saddr, eaddr, table);
    if (res < 0)
    {
        goto out;
    }

    res = multiheap_add_heap(multiheap, heap, flags);
    if (res < 0)
    {
        goto out;
    }

out:
    if (res < 0)
    {
        // TODO LATER.. CLEANUP!
    }
    return res;
}

void multiheap_free_heap(struct multiheap *multiheap)
{
    struct multiheap_single_heap *current = multiheap->first_multiheap;
    while (current != 0)
    {
        struct multiheap_single_heap *next = current->next;
        if (!(current->flags & MULTIHEAP_HEAP_FLAG_EXTERNALLY_OWNED))
        {
            heap_free(multiheap->starting_heap, current->heap);
        }

        current = next;
    }

    heap_free(multiheap->starting_heap, multiheap);
}

/**
 * Attempts to allocate the full size on a single heap.
 * within the multiheap.
 *
 * If it fails the first pass will return  NULL
 */
void *multiheap_alloc_first_pass(struct multiheap *multiheap, size_t size)
{
    void *allocation_ptr = NULL;
    struct multiheap_single_heap *current = multiheap->first_multiheap;
    while (current != 0)
    {
        allocation_ptr = heap_malloc(current->heap, size);
        if (allocation_ptr)
        {
            break;
        }
        current = current->next;
    }

    return allocation_ptr;
}

/**
 * Returns an address that can be used for paging purposes that is able to be pointed
 * to data of a given size without colliding with other data.
 *
 * Not to be written or read from until you have paged it correctly.
 * Keep function private
 *
 * eligible_heap_out will be set to the real heap that we can complete this with.
 *
 * THis design does have some limitations, that the defragmentation feature wont work across multiple heaps
 * a single heap will have to be chosen. I will comeback and improve that later...
 */
void *multiheap_alloc_paging(struct multiheap *multiheap, size_t size, struct multiheap_single_heap **eligible_heap_out)
{
    void *allocation_ptr = NULL;
    size_t total_required_blocks = size / PEACHOS_HEAP_BLOCK_SIZE;
    struct multiheap_single_heap *current = multiheap->first_multiheap;
    while (current != 0)
    {

        // Does this heap allow paging to solve fragmentation problems? 
        if (!multiheap_heap_allows_paging(current))
        {
            continue;
        }

        if (!current->paging_heap)
        {
            panic("Paging heap is NULL yet the heap allows paging");
        }
        // Firstly do we have enough free blocks in the real heap available to deal with the size
        // if we dont then we cant provide a paging mirror of the fragmented blocks
        // because we will never have enough real blocks to map to them later...
        if (current->heap->free_blocks < total_required_blocks)
        {
            // nah we cant deal with this heap move to the next one.
            current = current->next;
            continue;
        }

        // Ok we have a heap that does have enough real blocks available
        // for us to solve fragmentation issues using the paging heap.

        allocation_ptr = heap_malloc(current->paging_heap, size);
        if (allocation_ptr)
        {
            if (eligible_heap_out)
            {
                *eligible_heap_out = current;
            }
            break;
        }
        current = current->next;
    }

    

    // The allocation_ptr is unmapped memory, do not try to read or write from it
    // until it is mapped properly!!
    return allocation_ptr;
}

/**
 * Second chance allocation, will attempt to allocate memory over several heaps
 * and will use paging to defragment memory.
 *
 * This is a final chance allocation, if this fails then we are out of memory.
 */
void *multiheap_alloc_second_pass(struct multiheap *multiheap, size_t size)
{
    void *allocation_ptr = NULL;

    // Get the current paging descriptor that is used, probably the kernel descriptor
    struct paging_desc *paging_desc = paging_current_descriptor();
    if (!paging_desc)
    {
        panic("You must setup paging before defragmentation processes can occur...");
    }

    // Let's try to allocte enough memory on our paging heap, if we are successful we can map the entire
    // memory region to the real blocks we allocated on the real heap.
    // WARNING: DO NOT ACCESS THE MEMORY ALLOCATED IT EXISTS FOR PAGING REASONS ONLY
    // IT WILL PAGE FAULT

    // Align the size to a upper page incase they provide something not page aligned like "42"
    size = heap_align_value_to_upper(size);
    size_t total_blocks = size / PEACHOS_HEAP_BLOCK_SIZE;
    struct multiheap_single_heap *chosen_real_heap = NULL;

    // This point can be used as a virutal address to point to the real allocated memory
    // that we will soon allocate in the chosen real heap.
    // DO not access the memory until its mapped!!
    void *defragmented_virtual_memory_saddr = multiheap_alloc_paging(multiheap, size, &chosen_real_heap);
    if (!defragmented_virtual_memory_saddr)
    {
        // We couldnt find any room at all to defragment the heap
        allocation_ptr = NULL;
        goto out;
    }

    void *defragmented_virtual_memory_current_addr = defragmented_virtual_memory_saddr;
    // If allocationn does succeed then the start allocated address will be the virtual memory
    allocation_ptr = defragmented_virtual_memory_saddr;

    for (size_t i = 0; i < total_blocks; i++)
    {
        // one block at a time then map to the virtual memory, it will be continous
        // despite the real heap having the blocks spread.
        // add in failure checks later...
        void *block_addr = heap_zalloc(chosen_real_heap->heap, PEACHOS_HEAP_BLOCK_SIZE);
        if (!block_addr)
        {
            // Failed then panic because we already checked earlier
            // with multiheap_alloc_paging, if we was able to find a heap that would work
            panic("Something went wrong with allocating the blocks for second pass, despite enough blocks being available");
        }
        // Time to map the memory now..
        paging_map(paging_desc, defragmented_virtual_memory_current_addr, block_addr, PAGING_IS_WRITEABLE | PAGING_IS_PRESENT);
        // we need to add to the current addr virtual memory by one page
        defragmented_virtual_memory_current_addr += (uint64_t) PEACHOS_HEAP_BLOCK_SIZE;
    }


out:
    return allocation_ptr;
}

void *multiheap_alloc(struct multiheap *multiheap, size_t size)
{
    // Lets try to allocate with first pass first
    void *allocation_ptr = multiheap_alloc_first_pass(multiheap, size);
    if (allocation_ptr)
    {
        return allocation_ptr;
    }

    // normal alloc doesnt support paging defragmentation

    return NULL;
}

void *multiheap_palloc(struct multiheap *multiheap, size_t size)
{
    // Lets try to allocate with first pass first
    void *allocation_ptr = multiheap_alloc_first_pass(multiheap, size);
    if (allocation_ptr)
    {
        return allocation_ptr;
    }

    // NULL? then lets try a second pass
    allocation_ptr = multiheap_alloc_second_pass(multiheap, size);

    return allocation_ptr;
}

// multiheap free
void multiheap_free(struct multiheap *multiheap, void *ptr)
{
    struct multiheap_single_heap* paging_heap = NULL;
    struct multiheap_single_heap* phys_heap = NULL;
    void* real_phys_addr = NULL;

    // check that the ptr is page aligned
    //if ()
    // later..


    /**
     * paging_heap will be set to the heap that is responsible for paging for the given "phys" address
     * otherwise NULL if we havent used paging or defragmentation.
     * 
     * phys_heap will be set to the heap that is responsible for holding the real memory that was allocated
     * 
     * real_phys_addr will be set to the input "ptr" address if paging was not used, otherwise it is updated to the real address
     * as "ptr" would have been the virtual address in paging_heap not the address in phys_heap
     */
    multiheap_get_heap_and_paging_heap_for_address(multiheap, ptr, &phys_heap, &paging_heap, &real_phys_addr);
    
    if (paging_heap)
    {
        // Ptr will be virtual address in this caase as the paging_heap is set

        // How many blocks does this allocation take up? Becuase we need to loop through the page tables
        // so we can find the real physical blocks to free
        size_t total_blocks = heap_allocation_block_count(paging_heap->paging_heap, ptr);
        size_t starting_block = heap_address_to_block(paging_heap->paging_heap, ptr);
        size_t ending_block = starting_block+total_blocks;
        for (size_t i = starting_block; i < ending_block; i++)
        {
            // We need to know the physical address that the paging heap
            // is pointing too, for each block so we can free the real memory
            void* virtual_address_for_block = (void*) ((uintptr_t) ptr) + (i * PEACHOS_HEAP_BLOCK_SIZE);  
            void* data_phys_addr = paging_get_physical_address(paging_current_descriptor(), virtual_address_for_block);

            // We got the physical address, we can just call multiheap_free again on this address to clear it up
            multiheap_free(multiheap, data_phys_addr);
        }
        // MAKE SURE TO ACCESS PAGING_HEAP BECAUSE EACH PAGING HEAP HAS A REAL HEAP TOO!!!!!!
        heap_free(paging_heap->paging_heap, ptr);

        // Due to the event handlers in the paging heap the memory should have been unmapped correctly.
    } 
    else if (phys_heap)
    {
        // Now to free the real memory.
        heap_free(phys_heap->heap, real_phys_addr);
    }

    // Memory should now be freed...

}