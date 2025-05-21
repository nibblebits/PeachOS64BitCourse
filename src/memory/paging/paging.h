#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Enum to represent a paging map level
enum
{
    // 4 is default... 5 is optional
    PAGING_MAP_LEVEL_4 = 4,

    // NOTE: We only implement up to map level 4 for now..
    // map level 5 will not be considered.
};
typedef uint8_t paging_map_level_t;

#define PAGING_CACHE_DISABLED 0b00010000
#define PAGING_WRITE_THROUGH 0b00001000
#define PAGING_ACCESS_FROM_ALL 0b00000100
#define PAGING_IS_WRITEABLE 0b00000010
#define PAGING_IS_PRESENT 0b00000001

// Total entries per table is 512 in 64 bit mode
// This is for the page table array and the page directory
// and level 4
#define PAGING_TOTAL_ENTRIES_PER_TABLE 512

// We are still using 4kb pages
#define PAGING_PAGE_SIZE 4096

/**
 * The following explain the maximum memory mappable by each table part
 * you can use this knowledge to help calculate which entires in which tables to change
 * PML4T 0x80 0000 0000 (512 GB Max)
 *  PDPT 0x40000000 (1GB) MAX
 *  PD_TABLE: 0x200000 (2MB) MAX
 */

enum
{
    // 512 GB ( all 512 entries of the PLM4 map to 512 GB of memory maximum if all entries are filled)
    PAGING_PLM4T_MAX_ADDRESSABLE = 0x8000000000,
    // 1 GB
    PAGING_PDPT_MAX_ADDRESSABLE = 0x40000000,
    // 2 MB
    PAGING_PD_MAX_ADDRESSABLE = 0x200000,
    // 4096 bytes..
    PAGING_PAGE_MAX_ADDRESSABLE = PAGING_PAGE_SIZE,
};

/**
 * Structure for any paging entry thats part of paging.
 * For example this can be a page table entry or a page directory entry.
 * or even a page map level four entry. ect
 */
struct paging_desc_entry
{
    uint64_t present : 1;         // Bit 0: Present
    uint64_t read_write : 1;      // Bit 1: R/W
    uint64_t user_supervisor : 1; // Bit 2: U/S
    uint64_t pwt : 1;             // Bit 3: PWT
    uint64_t pcd : 1;             // Bit 4: PCD
    uint64_t accessed : 1;        // Bit 5: Accessed
    uint64_t ignored : 1;         // Bit 6: Ignored
    uint64_t reserved0 : 1;       // Bit 7: Must be 0 in PML4E

    uint64_t reserved1 : 4; // Bits 8-11: Reserved, must be 0

    uint64_t address : 40; // Bits 12-51: PDPT Base Address

    uint64_t available : 11; // Bits 52-62: Available to software

    uint64_t execute_disable : 1; // Bit 63: XD
} __attribute__((packed));

struct paging_pml_entries
{
    // each PML entry points to a page directory pointer table entry
    struct paging_desc_entry entries[PAGING_TOTAL_ENTRIES_PER_TABLE];
} __attribute__((packed));

/**
 * The root descriptor for page tables.
 */
struct paging_desc
{
    // Points to the first paging map level, likely page map level four.
    // unless page level map five is enabled
    struct paging_pml_entries *pml;

    // Indiciates weather the pml is Level 4 or 5
    paging_map_level_t level;

} __attribute__((packed));

/**
 * Creates a new paging descriptor which defines the root of the paging structures.
 *
 * \param root_map_level The maximum map level available to use. Only level 4 is supported by our kernel:  PAGING_MAP_LEVEL_4 .
 */
struct paging_desc *paging_desc_new(paging_map_level_t root_map_level);

/**
 * Frees the given descriptor, ensure your current paging descriptor is not the one you are freeing.
 * or your going to triple fault.
 */
void paging_desc_free(struct paging_desc* desc);


/**
 * Returns true if the entry is NULL or empty.
 */
bool paging_null_entry(struct paging_desc_entry *entry);



/**
 * Returns the current paging descriptor.
 */
struct paging_desc* paging_current_descriptor();

/**
 * Maps the provided virtual address to the physical address in memory that was provided,
 * the flags are then set on the page. This function will create all neccessary page maps, page directories
 * and page tables to ensure that the memory gets mapped correctly.
 *
 * "virt" and "phys" are expected to be 4096-byte page aligned addresses.
 *
 * \param desc  The paging descriptor
 * \param virt The virtual address to map
 * \param phys The physical address to map the virtual address too
 * \param flags The flags such as PAGING_IS_PRESENT, PAGING_IS_WRITEABLE ect..
 *
 * \return Returns zero if all went okay, negative if error, seek error codes for negative values.
 */
int paging_map(struct paging_desc *desc, void *virt, void *phys, int flags);

/**
 * Maps a range of pages
 */
int paging_map_range(struct paging_desc *desc, void *virt, void *phys, size_t count, int flags);

/**
 * Maps from the virtual address, physical address all the way to the end addresses
 */

int paging_map_to(struct paging_desc *desc, void *virt, void *phys, void *phys_end, int flags);

/**
 * Maps to all the available memory regions in the E820 memory map.
 * This function will map all usable memory regions in the E820 memory map to the paging descriptor.
 * 
 * Virtual address will map directly to the physical address.
 */
int paging_map_e820_memory_regions(struct paging_desc *desc);

/**
 * Aligns the given address to a 4096 page boundary.
 */
void* paging_align_address(void* ptr);

/**
 * Aligns the given address to a 4096 page boundary. Rounding down to the lower page if neccessary.
 */
void *paging_align_to_lower_page(void *addr);

void paging_switch(struct paging_desc *desc);

void paging_invalidate_tlb_entry(void *addr);

bool paging_is_aligned(void *addr);


/**
 * Gets the final descriptor entry for a given virtual address
 */
struct paging_desc_entry *paging_get(struct paging_desc *desc, void *virt);


/**
 * Gets the physical address in RAM for a given virtual addresss
 * The returend address is not page aligned, if you want page alignment then provide the page aligned
 * virtual address.
 */
void* paging_get_physical_address(struct paging_desc* desc, void* virtual_address);


// struct paging_4gb_chunk
// {
//     uint32_t* directory_entry;
// };

// struct paging_4gb_chunk* paging_new_4gb(uint8_t flags);
// void paging_switch(struct paging_4gb_chunk* directory);
// void enable_paging();

// int paging_set(uint32_t* directory, void* virt, uint32_t val);

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