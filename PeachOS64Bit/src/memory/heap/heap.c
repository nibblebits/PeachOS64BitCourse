#include "heap.h"
#include "kernel.h"
#include "status.h"
#include "memory/memory.h"
#include <stdbool.h>
#include <stdint.h>

static int heap_validate_table(void *ptr, void *end, struct heap_table *table)
{
    int res = 0;

    size_t table_size = (size_t)(end - ptr);
    size_t total_blocks = table_size / PEACHOS_HEAP_BLOCK_SIZE;
    if (table->total != total_blocks)
    {
        res = -EINVARG;
        goto out;
    }

out:
    return res;
}

static bool heap_validate_alignment(void *ptr)
{
    return ((uintptr_t)ptr % PEACHOS_HEAP_BLOCK_SIZE) == 0;
}

int heap_create(struct heap *heap, void *ptr, void *end, struct heap_table *table)
{
    int res = 0;

    if (!heap_validate_alignment(ptr) || !heap_validate_alignment(end))
    {
        res = -EINVARG;
        goto out;
    }

    memset(heap, 0, sizeof(struct heap));
    heap->saddr = ptr;
    heap->eaddr = end;
    heap->table = table;
    heap->total_blocks = table->total;
    heap->free_blocks = table->total;
    heap->used_blocks = 0;

    res = heap_validate_table(ptr, end, table);
    if (res < 0)
    {
        goto out;
    }

    size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
    memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);

out:
    return res;
}

uintptr_t heap_align_value_to_upper(uintptr_t val)
{
    if ((val % PEACHOS_HEAP_BLOCK_SIZE) == 0)
    {
        return val;
    }

    val = (val - (val % PEACHOS_HEAP_BLOCK_SIZE));
    val += PEACHOS_HEAP_BLOCK_SIZE;
    return val;
}

uintptr_t heap_align_value_to_lower(uintptr_t val)
{
    // Check if the value is already aligned
    if ((val % PEACHOS_HEAP_BLOCK_SIZE) == 0)
    {
        return val;
    }

    // Subtract the remainder
    val = val - (val % PEACHOS_HEAP_BLOCK_SIZE);
    return val;
}

static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
    return entry & 0x0f;
}

bool heap_is_address_within_heap(struct heap* heap, void* ptr)
{
    return (ptr >= heap->saddr && ptr <= heap->eaddr);
}

void heap_callbacks_set(struct heap* heap, HEAP_BLOCK_ALLOCATED_CALLBACK_FUNCTION allocated_func, HEAP_BLOCK_FREE_CALLBACK_FUNCTION free_func)
{
    heap->block_allocated_callback = allocated_func;
    heap->block_free_callback = free_func;
}

int64_t heap_get_start_block(struct heap *heap, uintptr_t total_blocks)
{
    struct heap_table *table = heap->table;
    int64_t bc = 0;
    int64_t bs = -1;

    for (size_t i = 0; i < table->total; i++)
    {
        if (heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE)
        {
            bc = 0;
            bs = -1;
            continue;
        }

        // Is this the first block?
        if (bs == -1)
        {
            bs = i;
        }

        bc++;
        if (bc == total_blocks)
        {
            break;
        }
    }

    // If this is the first block
    if (bc != total_blocks)
    {
        return -ENOMEM;
    }

    return bs;
}


bool heap_is_block_range_free(struct heap* heap, size_t starting_block, size_t ending_block)
{
    struct heap_table* table = heap->table;
    for(size_t i = starting_block; i <= ending_block; i++)
    {
        if(table->entries[i] & HEAP_BLOCK_TABLE_ENTRY_TAKEN)
        {
            return false;
        }
    }

    return true;
}
size_t heap_allocation_block_count(struct heap* heap, void* starting_address)
{
    size_t count = 0;
    struct heap_table* heap_table = heap->table;
    int64_t starting_block = heap_address_to_block(heap, starting_address);
    if (starting_block < 0)
    {
        goto out;
    }

    for (int64_t i = starting_block; i < (int64_t) heap_table->total; i++)
    {
        HEAP_BLOCK_TABLE_ENTRY entry = heap_table->entries[i];
        if (entry & HEAP_BLOCK_TABLE_ENTRY_TAKEN)
        {
            count++;
        }

        // End of this block chain?
        if (!(entry & HEAP_BLOCK_HAS_NEXT))
        {
            break;
        }
    }

out:
    return count;
}

void *heap_block_to_address(struct heap *heap, int64_t block)
{
    return heap->saddr + (block * PEACHOS_HEAP_BLOCK_SIZE);
}

void heap_mark_blocks_taken(struct heap *heap, int64_t start_block, int64_t total_blocks)
{
    int64_t end_block = (start_block + total_blocks) - 1;

    HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;
    if (total_blocks > 1)
    {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    for (int64_t i = start_block; i <= end_block; i++)
    {
        heap->table->entries[i] = entry;
        entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;
        if (i != end_block)
        {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }

        void* address = heap_block_to_address(heap, i);
        if(heap->block_allocated_callback)
        {
            heap->block_allocated_callback(address, PEACHOS_HEAP_BLOCK_SIZE);
        }
    }
}

void *heap_malloc_blocks(struct heap *heap, uintptr_t total_blocks)
{
    void *address = 0;

    int64_t start_block = heap_get_start_block(heap, total_blocks);
    if (start_block < 0)
    {
        goto out;
    }

    address = heap_block_to_address(heap, start_block);

    // Mark the blocks as taken
    heap_mark_blocks_taken(heap, start_block, total_blocks);

    heap->used_blocks += total_blocks;
    heap->free_blocks -= total_blocks;

out:
    return address;
}

void heap_mark_blocks_free(struct heap *heap, int64_t starting_block)
{
    struct heap_table *table = heap->table;
    size_t total_blocks_freed = 0;
    for (int64_t i = starting_block; i < (int64_t)table->total; i++)
    {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;
        void* address = heap_block_to_address(heap, i);
        if(heap->block_free_callback)
        {
            heap->block_free_callback(address);
        }

        total_blocks_freed++;
        if (!(entry & HEAP_BLOCK_HAS_NEXT))
        {
            break;
        }
    }

    heap->used_blocks -= total_blocks_freed;
    heap->free_blocks += total_blocks_freed;
}

int64_t heap_address_to_block(struct heap *heap, void *address)
{
    return ((int64_t)(address - heap->saddr)) / PEACHOS_HEAP_BLOCK_SIZE;
}

void *heap_malloc(struct heap *heap, size_t size)
{
    size_t aligned_size = heap_align_value_to_upper(size);
    int64_t total_blocks = aligned_size / PEACHOS_HEAP_BLOCK_SIZE;
    return heap_malloc_blocks(heap, total_blocks);
}

void* heap_realloc(struct heap* heap, void* old_ptr, size_t new_size)
{
    // NULL pointer then fresh allocation
    if (!old_ptr)
    {
        return heap_malloc(heap, new_size);
    }

    if (new_size == 0)
    {
        heap_free(heap, old_ptr);
        return NULL;
    }

    // Get the current allocations block count and starting block
    size_t current_alloc_blocks = heap_allocation_block_count(heap, old_ptr);
    int64_t starting_block = heap_address_to_block(heap, old_ptr);
    // Calculate ending block index
    int64_t ending_block = starting_block + current_alloc_blocks -1;

    // ALign the new requested size
    size_t new_size_aligned = heap_align_value_to_upper(new_size);
    // Determine how many blocks are needed for the new allocation
    size_t new_total_blocks = new_size_aligned / PEACHOS_HEAP_BLOCK_SIZE;
    size_t old_total_size = current_alloc_blocks * PEACHOS_HEAP_BLOCK_SIZE;

    // Do we need to shrink the allocation
    if (current_alloc_blocks >= new_total_blocks)
    {
        // Is it the same requested size as the memory size?
        // then return the old pointer
        if (current_alloc_blocks == new_total_blocks)
        {
            return old_ptr;
        }

        int64_t block_to_free = starting_block + new_total_blocks;
        // Free all blocks from block_to_free to the current allocations end
        heap_mark_blocks_free(heap, block_to_free);

        if (new_total_blocks > 0)
        {
            heap->table->entries[starting_block + new_total_blocks-1] &= ~HEAP_BLOCK_HAS_NEXT;
        }

        // Adjust the counts
        size_t freed_blocks = current_alloc_blocks - new_total_blocks;
        heap->used_blocks -= freed_blocks;
        heap->free_blocks += freed_blocks;
        return old_ptr;
    }

    // Expand the allocation
    size_t extra_blocks = new_total_blocks - current_alloc_blocks;
    size_t extension_start = ending_block +1;
    size_t extension_end = extension_start + extra_blocks -1;
    if(heap_is_block_range_free(heap, extension_start, extension_end))
    {
        // Mark all the extension blocks as taken
        for(size_t i = extension_start; i < extension_end; i++)
        {
            heap->table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_HAS_NEXT;
        }

        // Mark the final block of the extension as taken
        heap->table->entries[extension_end] = HEAP_BLOCK_TABLE_ENTRY_TAKEN;
        // Ensure that the old ending block is marked with has next
        heap->table->entries[ending_block] |= HEAP_BLOCK_HAS_NEXT;

        // Adjust block counts.
        heap->used_blocks += extra_blocks;
        heap->free_blocks -= extra_blocks;
        return old_ptr;
    }

    // We are unable to extend the allocation, due to additional mallocs
    // that have taken place, breaking the free block chain ahead of us
    // the final resort, is to copy all the memory into a new allocation

    void* new_addr = heap_zalloc(heap, new_size_aligned);
    if(!new_addr)
    {
        // Out of memory
        return NULL;
    }

    // Copy the old data into the new allocation
    memcpy(new_addr, old_ptr, old_total_size);

    // Free the old pointer
    heap_free(heap, old_ptr);
    return new_addr;
}

void heap_free(struct heap *heap, void *ptr)
{
    heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}

size_t heap_total_size(struct heap *heap)
{
    return heap->table->total * PEACHOS_HEAP_BLOCK_SIZE;
}

size_t heap_total_used(struct heap *heap)
{
    size_t total = 0;
    struct heap_table *table = heap->table;
    for (size_t i = 0; i < table->total; i++)
    {
        if (heap_get_entry_type(table->entries[i]) == HEAP_BLOCK_TABLE_ENTRY_TAKEN)
        {
            total += PEACHOS_HEAP_BLOCK_SIZE;
        }
    }

    return total;
}

size_t heap_total_available(struct heap *heap)
{
    return heap_total_size(heap) - heap_total_used(heap);
}

void *heap_zalloc(struct heap *heap, size_t size)
{
    void *ptr = heap_malloc(heap, size);
    if (!ptr)
    {
        return 0;
    }

    memset(ptr, 0x00, size);
    return ptr;
}