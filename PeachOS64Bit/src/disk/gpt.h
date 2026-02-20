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