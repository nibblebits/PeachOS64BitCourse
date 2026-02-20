/*
 * PeachOS 64-Bit Kernel Project
 * Copyright (C) 2026 Daniel McCarthy <daniel@dragonzap.com>
 *
 * This file is part of the PeachOS 64-Bit Kernel.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License version 2 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 * For full source code, documentation, and structured learning,
 * see the official kernel development course part one:
 * https://dragonzap.com/course/developing-a-multithreaded-kernel-from-scratch

 * Get part one and part two module one, module two all peachos courses (69 hours of content): https://dragonzap.com/offer/kernel-development-from-scratch-69-hours

 * Get the part two, module one and two modules: https://dragonzap.com/offer/developing-a-multithreaded-kernel-from-scratch-part-two-full-series
 */

#include "lib/vector/vector.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"

#include <stddef.h>
#include <stdint.h>

static off_t vector_offset(struct vector *vec, off_t index)
{
    return (off_t)vec->e_size * index;
}

static int vector_valid_bounds(struct vector *vec, size_t index)
{
    return (index < vec->t_elems) ? 0 : -EINVARG;
}

static void *vector_memory_at_index(struct vector *vec, size_t index)
{
    // We don't validate bounds here because it might be a write.
    off_t offset = vector_offset(vec, index);
    return (void *)((uintptr_t)vec->memory + offset);
}

/**
 * Creates a new vector that can hold "element_size" per element of data
 * \param element_size The size of each element to be stored in the vector
 * \param total_reserved_elements_per_resize When vector is out of memory and needs resize, set this to how many elements should we reserve
 * \param flags The vector flags to determine how it should function
 */
struct vector *vector_new(size_t element_size, size_t total_reserved_elements_per_resize, int flags)
{
    struct vector *vec = kpzalloc(sizeof(struct vector));
    if (!vec)
    {
        // out of mem
        return NULL;
    }
    vec->e_size = element_size;
    vec->flags = flags;
    vec->t_elems = 0;
    vec->t_reserved_elements = total_reserved_elements_per_resize;

    // Total elements currently in memory but not used, they are reserved
    vec->tm_elems = 0;

    vec->memory = NULL;

    return vec;
}

/**
 * Resizes the vector if we cannot fit the total_needed_elements,
 * invalidates any older pointers important to remember that
 */
int vector_resize(struct vector *vec, size_t total_needed_elements)
{
    int res = 0;
    size_t total_elements_required = vec->t_elems + total_needed_elements;
    // Have we already got the memory allocated for the needed elementes
    if (vec->tm_elems > total_elements_required)
    {
        // We have enough room already, no resize needed
        return 0;
    }

    // We need to allocate
    size_t final_total_elements_required = vec->t_reserved_elements + total_elements_required;
    size_t total_bytes_required = final_total_elements_required * vec->e_size;
    vec->memory = krealloc(vec->memory, total_bytes_required);
    if (!vec->memory)
    {
        res = -ENOMEM;
        goto out;
    }

    // Let's record the increase in size
    vec->tm_elems = final_total_elements_required;

    // Vector has been resized to be able to hold an additional total_needed_elements
    // We wont ever bother to shrink memory howeveer.. only to increase it.

out:
    return res;
}

/**
 * Adds an element to the vector.
 * The memory at "elem" is copied into the vectors memory
 *
 * \param vec The vector to push the element too
 * \param elem The address of the element to add to the vector
 * \return Returns the index of the newly created element or negative on error
 */
int vector_push(struct vector *vec, void *elem)
{
    int res = 0;
    // Maybe a resize is needed
    res = vector_resize(vec, 1);
    if (res < 0)
    {
        goto out;
    }

    // Reply will be the index we wrote too assuming no error
    res = vec->t_elems;

    // The current index will be vec->t_elems as this represents the array index
    // of where the next write should be, since t_elems represents the current count.
    memcpy(vector_memory_at_index(vec, vec->t_elems), elem, vec->e_size);

    // Next we need to increase the total elements by one as we have
    // copied the data
    vec->t_elems++;

out:
    return res;
}

int vector_overwrite(struct vector *vec, int index, void *elem, size_t elem_size)
{
    int res = 0;
    res = vector_valid_bounds(vec, index);
    if (res < 0)
    {
        goto out;
    }

    // Did the user provide the expected size? If not then
    // we know the caller doesnt understand the vector
    // to prevent a bug we should fail
    // the size field exists as a safety precaution so we dont overflow buffers.
    if (elem_size < vec->e_size)
    {
        res = -EINVARG;
        goto out;
    }

    // Perform the overwrite.
    memcpy(vector_memory_at_index(vec, index), elem, vec->e_size);

out:
    return res;
}
/**
 * Removes an element from the vector
 * \param vec The vector to pop the element from
 */
int vector_pop(struct vector *vec)
{
    if (vec->t_elems == 0)
    {
        return -EIO;
    }

    // Pop action is very simple, we must just decrement by one
    // from the current elements
    vec->t_elems--;

    return 0;
}

int vector_back(struct vector *vec, void *data_out, size_t size)
{
    // "t_elems" is a count of how many elements are in the vector
    // Minus 1 to convert to array index of the last element.
    return vector_at(vec, vec->t_elems - 1, data_out, size);
}

void vector_reorder(struct vector* vec, VECTOR_REORDER_FUNCTION reorder_function)
{
    // Sanity checks
    if (!vec || !reorder_function)
        return;

    size_t count = vector_count(vec);
    // If there's 0 or 1 element, there's nothing to reorder
    if (count < 2)
        return;

    // We'll do a simple bubble sort
    // For that, we need temporary buffers to hold elements while swapping
    uint8_t* elem1 = kzalloc(vec->e_size);
    uint8_t* elem2 = kzalloc(vec->e_size);

    if (!elem1 || !elem2)
    {
        if (elem1) kfree(elem1);
        if (elem2) kfree(elem2);
        return; // Out of memory
    }

    for (size_t i = 0; i < count - 1; i++)
    {
        for (size_t j = 0; j < count - i - 1; j++)
        {
            // Read element j
            if (vector_at(vec, j, elem1, vec->e_size) < 0)
                goto cleanup;

            // Read element j+1
            if (vector_at(vec, j + 1, elem2, vec->e_size) < 0)
                goto cleanup;

            // If reorder_function(...) > 0, then we swap
            // Typical meaning: reorder_function(a, b) > 0 implies a > b, so we reorder them
            if (reorder_function(elem1, elem2) > 0)
            {
                // Swap in the vector
                // (Write elem2 at index j)
                if (vector_overwrite(vec, j, elem2, vec->e_size) < 0)
                    goto cleanup;
                // (Write elem1 at index j+1)
                if (vector_overwrite(vec, j + 1, elem1, vec->e_size) < 0)
                    goto cleanup;
            }
        }
    }

cleanup:
    kfree(elem1);
    kfree(elem2);
}


/**
 * Finds the element at the given index and writes it to "data_out"
 * returns zero on success, negative when theres a problem..
 * \param vec The vector to retreive the data from
 * \param index The index in the vector that you wish to extract the element from
 * \param data_out a pointer to a buffer we can copy the value too
 * \param size The size you expect the value to be, if it differs from vector element size error will be returned, this value is required for safety purposes to ensure the caller understands the elements being read.
 * \return Returns zero on success, negative on error
 */
int vector_at(struct vector *vec, size_t index, void *data_out, size_t size)
{
    int res = 0;

    // Did the user provide the expected size? If not then
    // we know the caller doesnt understand the vector
    // to prevent a bug we should fail
    // the size field exists as a safety precaution so we dont overflow buffers.
    if (size < vec->e_size)
    {
        res = -EINVARG;
        goto out;
    }

    if (!data_out)
    {
        res = -EINVARG;
        goto out;
    }

    // Validate the index is correct first.
    res = vector_valid_bounds(vec, index);
    if (res < 0)
    {
        goto out;
    }

    // Bounds are validated, we can safely read the data
    memcpy(data_out, vector_memory_at_index(vec, index), vec->e_size);

    // Data now copied correctly
    // Why dont we just return a pointer?

    // because realloc has the chance to change the whole memory region
    // thus invalidating all previous pointers held by the caller.
out:
    if (res < 0)
    {
        // error then just set the data_out as null bytes
        memset(data_out, 0, size);
    }
    return res;
}

/**
 * Pops the element with the given value from the vector.
 * 
 * The mem_val pointer is compared against all values in the vector, once a match is found
 * that element is removed. Other elements may be shifted.
 * \param mem_val pointer to the memory that equals a value that is stored in the vector to be deleted
 * \param size THe size of the vector element
 */
int vector_pop_element(struct vector* vec, void* mem_val, size_t size)
{
    int res = 0;

    // Validate size matches the vector element size
    if (size != vec->e_size)
    {
        return -EINVARG;
    }

    size_t total_elements = vector_count(vec);
    long index_to_remove = -1;

    // Temporary buffer for comparison
    void* tmp_mem = kzalloc(size);
    if (!tmp_mem)
    {
        return -ENOMEM;
    }

    // Search for the element in the vector
    for (size_t i = 0; i < total_elements; i++)
    {
        res = vector_at(vec, i, tmp_mem, size);
        if (res < 0)
        {
            // Error reading element
            goto out;
        }

        // If we found the element, remember its index and break
        if (memcmp(tmp_mem, mem_val, size) == 0)
        {
            index_to_remove = i;
            break;
        }
    }

    // If the element wasn't found, bail out
    if (index_to_remove == -1)
    {
        res = -EIO;
        goto out;
    }

    // If the element to remove is the last one, a normal pop is enough
    if ((size_t)index_to_remove == total_elements - 1)
    {
        res = vector_pop(vec);
        goto out;
    }

    // Otherwise, shift elements after index_to_remove to the left
    {
        uint8_t* ptr_to_element_to_delete = (uint8_t*)vec->memory + (index_to_remove * size);
        uint8_t* ptr_to_next_element     = ptr_to_element_to_delete + size;

        size_t total_elements_to_copy = (vec->t_elems - index_to_remove) - 1;
        size_t total_bytes_to_copy    = total_elements_to_copy * size;

        // Shift everything after index_to_remove one position to the left
        memcpy(ptr_to_element_to_delete, ptr_to_next_element, total_bytes_to_copy);

        // Decrement the logical element count
        vec->t_elems--;
    }

out:
    kfree(tmp_mem);
    return res;
}

/**
 * Returns the total number of elements within this vector
 * \param vec The vector to element count
 * \return the total count
 */
size_t vector_count(struct vector *vec)
{
    return vec->t_elems;
}

void vector_free(struct vector *vec)
{
    if (vec->memory)
    {
        kfree(vec->memory);
    }
    kfree(vec);
}