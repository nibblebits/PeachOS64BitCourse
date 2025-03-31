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
    heap->table = table;

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

static uintptr_t heap_align_value_to_upper(uintptr_t val)
{
    if ((val % PEACHOS_HEAP_BLOCK_SIZE) == 0)
    {
        return val;
    }

    val = (val - (val % PEACHOS_HEAP_BLOCK_SIZE));
    val += PEACHOS_HEAP_BLOCK_SIZE;
    return val;
}

static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
    return entry & 0x0f;
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

out:
    return address;
}

void heap_mark_blocks_free(struct heap *heap, int64_t starting_block)
{
    struct heap_table *table = heap->table;
    for (int64_t i = starting_block; i < (int64_t)table->total; i++)
    {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;
        if (!(entry & HEAP_BLOCK_HAS_NEXT))
        {
            break;
        }
    }
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