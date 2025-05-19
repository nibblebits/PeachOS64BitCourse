#include "gdt.h"
#include "memory/memory.h"
#include "kernel.h"

void gdt_set(struct gdt_entry* gdt_entry, void* address, uint16_t limit_low, uint8_t access_byte, uint8_t flags)
{
    if ((uintptr_t) address > 0xffffffff)
    {
        panic("Address must be within a 32 bit range for gdt entry\n");
    }

    uint32_t base = (uint32_t)(uintptr_t) address;
    gdt_entry->base_first = base & 0xffff; // Set the low 16 bits of the base
    gdt_entry->base = (base >> 16) & 0xFF; // Set next 8 bits of the base
    gdt_entry->base_24_31_bits = (base >> 24) & 0xFF; // FInal 8 bits of the base

    gdt_entry->segment = limit_low;   // Set the segment limit
    gdt_entry->access = access_byte;  // Set the access byte
    gdt_entry->high_flags = flags;
}

void gdt_set_tss(struct tss_desc_64* desc, void* tss_addr, uint16_t limit, uint8_t type, uint8_t flags)
{
    memset(desc, 0, sizeof(struct tss_desc_64));
    
    // Lower 16 bits
    desc->limit0 = limit & 0xFFFF;

    // Limit high nibble goes in bits 48..51
    desc->limit1_flags = (uint8_t) ((limit >> 16) & 0x0F);

    // 64 bit base
    uint64_t base = (uint64_t) tss_addr;
    desc->base0 = (uint16_t) (base & 0xFFFF); // bits 0..15
    desc->base1 = (uint8_t) ((base >> 16) & 0xFF); // bits 16..23
    desc->base2 = (uint8_t)((base >> 24) & 0xFF); // bits 24..31
    desc->base3 = (uint32_t)((base >> 32) & 0xFFFFFFFF); // bits 32..63

    // Type byte (bits 40..47) includes Present and available 64-bit tss bits.
    desc->type = type; // Typically be 0x89

    desc->reserved = 0;
}