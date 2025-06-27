#ifndef KERNEL_VECTOR_H
#define KERNEL_VECTOR_H
#include <stddef.h>
#include <stdint.h>
#include "types.h"

typedef int (*VECTOR_REORDER_FUNCTION)(void* first_element, void* second_element);

enum
{
    VECTOR_NO_FLAGS = 0
};


struct vector
{
    // Vector memory
    void* memory;

    // ANy flags associated with the vector;
    int flags;

    // Size of a single element in the vector
    size_t e_size;

    // Total elements in this vector
    size_t t_elems;

    // Total elements that can fit in the vector currently
    // if t_elems == tm_elems then we need to resize the vector
    size_t tm_elems;


    // Total reserved elements per resize
    size_t t_reserved_elements;
};


/**
 * Creates a new vector that can hold "element_size" per element of data
 * \param element_size The size of each element to be stored in the vector
 * \param total_reserved_elements_per_resize When vector is out of memory and needs resize, set this to how many elements should we reserve
 * \param flags The vector flags to determine how it should function
 */
struct vector* vector_new(size_t element_size, size_t total_reserved_elements_per_resize, int flags);

/**
 * Frees the vector memory
 */
void vector_free(struct vector* vec);

/**
 * Adds an element to the vector.
 * The memory at "elem" is copied into the vectors memory
 * 
 * \param vec The vector to push the element too
 * \param elem The address of the element to add to the vector
 * \return Returns the index of the newly created element or negative on error
 */
int vector_push(struct vector* vec, void* elem);

/**
 * Removes an element from the vector
 * \param vec The vector to pop the element from
 */
int vector_pop(struct vector* vec);

/**
 * Reorders the vector.
 */
void vector_reorder(struct vector* vec, VECTOR_REORDER_FUNCTION reorder_function);


/**
 * Overwrites an element in the vector at the given index
 * \param vec The vector to write too
 * \param index THe index in the vector to overwrite
 * \param elem THe pointer to the element
 * \param elem_size The size of the element, MUST MATCH THE VECTOR ELEMENT SIZE
 */
int vector_overwrite(struct vector *vec, int index, void *elem, size_t elem_size);
/**
 * Returns a pointer to the memory of the last pushed element to this vector.
 * 
 * You remain responsible for the memory at data_out
 * 
 * \param vec The vector 
 * \param data_out The area to write memory for the last element
 * \param size The size of the element your expecting
 * \return Returns zero on success, negative on error
 */
int vector_back(struct vector* vec, void* data_out, size_t size);


/**
 * Finds the element at the given index and writes it to "data_out"
 * returns zero on success, negative when theres a problem..
 * \param vec The vector to retreive the data from
 * \param index The index in the vector that you wish to extract the element from
 * \param data_out a pointer to a buffer we can copy the value too
 * \param size The size you expect the value to be, if it differs from vector element size error will be returned, this value is required for safety purposes to ensure the caller understands the elements being read.
 * \return Returns zero on success, negative on error
 */
int vector_at(struct vector* vec, size_t index, void* data_out, size_t size);


/**
 * Returns the total number of elements within this vector
 * \param vec The vector to element count
 * \return the total count
 */
size_t vector_count(struct vector* vec);


/**
 * Pops the element with the given value from the vector.
 * 
 * The mem_val pointer is compared against all values in the vector, once a match is found
 * that element is removed. All other elements will be shifted to the left
 * and the vector count deducted by 1
 * \param mem_val pointer to the memory that equals a value that is stored in the vector to be deleted
 * \param size THe size of the vector element
 */
int vector_pop_element(struct vector* vec, void* mem_val, size_t size);


#endif