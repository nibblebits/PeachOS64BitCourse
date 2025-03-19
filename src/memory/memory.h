#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

struct e820_entry
{
    uint64_t base_addr; // Starting address of the memory region
    uint64_t length;    // Size of the region in bytes
    uint32_t type;      // Type of the memory region (1 = usable, others=reserved, ect...)
    uint32_t extended_attr; // Extended attributes (often zero)
} __attribute__((packed));

size_t e820_total_accessible_memory();
struct e820_entry* e820_largest_free_entry();
size_t e820_total_entries();
struct e820_entry* e820_entry(size_t index);

void* memset(void* ptr, int c, size_t size);
int memcmp(void* s1, void* s2, int count);
void* memcpy(void* dest, void* src, int len);

#endif