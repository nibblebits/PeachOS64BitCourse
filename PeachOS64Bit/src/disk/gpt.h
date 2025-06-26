#ifndef KERNEL_GPT_H
#define KERNEL_GPT_H

#include <stdint.h>
#include <stddef.h>

#define GPT_MASTER_BOOT_RECORD_LBA 0
#define GPT_PARTITION_TABLE_HEADER_LBA 1
#define GPT_SIGNATURE "EFI PART"

struct gpt_partition_table_header
{
    char signature[8];
    uint32_t revision;
    uint32_t hdr_size;
    uint32_t cr32_checksum;
    uint32_t reserved1;
    // The LBA that contains this header
    uint64_t lba;
    // The lba of the alternate gpt header
    uint64_t lba_alternate;

    // First LBA to the partition data
    uint64_t data_block_lba;
    // The ending block LBA
    uint64_t data_block_lba_end;

    // GUID identifier of the disk
    char guid[16];

    uint64_t guid_array_lba_start;
    // Total entries in the partition
    uint32_t total_array_entries;
    // The total size of each entry in the parititon array
    uint32_t array_entry_size;

    // The CRC32 of the p arition table entry array
    uint32_t crc32_entry_array;


    // Blocksize-0x5C
    char reserved2[];
};

struct gpt_partition_entry
{
    char guid[16];
    char unique_partition_guid[16];
    uint64_t starting_lba;
    uint64_t ending_lba;
    uint64_t attributes;
    char partition_name[72];
};

int gpt_init();

#endif