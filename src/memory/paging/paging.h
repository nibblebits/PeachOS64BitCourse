#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

enum
{
    PAGING_MAP_LEVEL_4 = 4,
    // 5 is not yet supported
};
typedef uint8_t paging_map_level_t;

#define PAGING_CACHE_DISABLED  0b00010000
#define PAGING_WRITE_THROUGH   0b00001000
#define PAGING_ACCESS_FROM_ALL 0b00000100
#define PAGING_IS_WRITEABLE    0b00000010
#define PAGING_IS_PRESENT      0b00000001


#define PAGING_TOTAL_ENTRIES_PER_TABLE 512

// 4K pages.
#define PAGING_PAGE_SIZE 4096

enum
{
    // 512 GB Aprox (all 512 entries of the PLM4 map to 512 GB of maximum memory
    // if all entries are filled)
    PAGING_PLM4T_MAX_ADDRESSABLE = 0x8000000000,
    // 1 GB
    PAGING_PDPT_MAX_ADDRESSABLE = 0x40000000,
    // 2 MB
    PAGING_PD_MAX_ADDRESSABLE = 0x200000,
    // 4096 bytes
    PAGING_PAGE_MAX_ADDRESSABLE = PAGING_PAGE_SIZE
};


/**
 * Structure for any paging entry thats part of paging.
 * For example this can be a page table entry, a page directory entry
 * or even a page map level four entry.
 */
struct paging_desc_entry
{
    uint64_t present : 1;   // Bit 0: Present
    uint64_t read_write : 1; // Bit 1: R/W
    uint64_t user_supervisor : 1; // Bit 2: U/S
    uint64_t pwt : 1;             // Bit 3: PWT
    uint64_t pcd : 1;             // Bit 4: PCD
    uint64_t accessed : 1;        // Bit 5: Accessed
    uint64_t ignored : 1;         // Bit 6: Ignored
    uint64_t reserved0 : 1;       // Bit 7: Must be 0 in PML4E
    uint64_t reserved1 : 4;       // Bits 8:11: Reserved must be 0
    uint64_t address   : 40;      // Bits 12-51: PDPT Base address
    uint64_t available : 11;      // Bits 52-62 Available to software
    uint64_t execute_disable : 1; // Bit 63: XD
} __attribute__((packed));

struct paging_pml_entries
{   
    struct paging_desc_entry entries[PAGING_TOTAL_ENTRIES_PER_TABLE];
} __attribute__((packed));

struct paging_desc
{
    // pml4 for this kernel ALWAYS
    struct paging_pml_entries* pml;

    // Indiciates weather the pml is level 4 or 5 or a future level.
    paging_map_level_t level;
} __attribute__((packed));

int paging_map_to(struct paging_desc* desc, void* virt, void* phys, void* phys_end, int flags);
int paging_map_range(struct paging_desc* desc, void* virt, void* phys, size_t count, int flags);
int paging_map(struct paging_desc* desc, void* virt, void* phys, int flags);
void* paging_align_to_lower_page(void* addr);
void* paging_align_address(void* ptr);
struct paging_desc* paging_desc_new(paging_map_level_t root_map_level);

void paging_load_directory(uintptr_t* directory);
void paging_invalidate_tlb_entry(void* addr);
void paging_switch(struct paging_desc* desc);



// struct paging_4gb_chunk
// {
//     uint32_t* directory_entry;
// };

// struct paging_4gb_chunk* paging_new_4gb(uint8_t flags);
// void paging_switch(struct paging_4gb_chunk* directory);
// void enable_paging();

// int paging_set(uint32_t* directory, void* virt, uint32_t val);
// bool paging_is_aligned(void* addr);

// uint32_t* paging_4gb_chunk_get_directory(struct paging_4gb_chunk* chunk);
// void paging_free_4gb(struct paging_4gb_chunk* chunk);

// int paging_map_to(struct paging_4gb_chunk *directory, void *virt, void *phys, void *phys_end, int flags);
// int paging_map_range(struct paging_4gb_chunk* directory, void* virt, void* phys, int count, int flags);
// int paging_map(struct paging_4gb_chunk* directory, void* virt, void* phys, int flags);
// void* paging_align_address(void* ptr);
// uint32_t paging_get(uint32_t* directory, void* virt);
// void* paging_align_to_lower_page(void* addr);
// void* paging_get_physical_address(uint32_t* directory, void* virt);

#endif