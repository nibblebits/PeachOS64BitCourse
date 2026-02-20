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

#include "gpt.h"
#include "disk/disk.h"
#include "status.h"
#include "memory/memory.h"
#include "disk/streamer.h"
#include "kernel.h"

struct disk* gpt_primary_disk = NULL;

size_t gpt_partition_table_header_real_size(struct gpt_partition_table_header* header)
{
    return sizeof(*header) + (header->hdr_size - offsetof(struct gpt_partition_table_header, reserved2));
}

int gpt_partition_table_header_read(struct gpt_partition_table_header* header_out)
{
    int res = 0;
    char sector[gpt_primary_disk->sector_size];
    res = disk_read_block(gpt_primary_disk, GPT_PARTITION_TABLE_HEADER_LBA, 1, sector);
    if (res < 0)
    {
        goto out;
    }

    memcpy(header_out, sector, sizeof(*header_out));
out:
    return res;
}

int gpt_mount_partitions(struct gpt_partition_table_header* partition_header)
{
    int res = 0;
    size_t total_entries = partition_header->total_array_entries;
    uint64_t starting_lba = partition_header->guid_array_lba_start;
    uint64_t starting_byte = starting_lba * gpt_primary_disk->sector_size;
    size_t entry_size = partition_header->array_entry_size;
    struct disk_stream* streamer = diskstreamer_new(gpt_primary_disk->id);
    if (!streamer)
    {
        res = -EINVARG;
        goto out;
    }

    res = diskstreamer_seek(streamer, (int) starting_byte);
    if (res < 0)
    {
        goto out;
    }

    for (size_t i = 0; i < total_entries; i++)
    {
        // Read a single partition entry
        char buffer[entry_size];
        res = diskstreamer_read(streamer, buffer, sizeof(buffer));
        if (res < 0)
        {
            goto out;
        }

        // Cast to access the memory as it truly is
        struct gpt_partition_entry* entry = (struct gpt_partition_entry*) buffer;
        char guid_empty[16] = {0};
        // If the GUID is all zeros then this entry can be skipped
        if (memcmp(entry->guid, guid_empty, sizeof(entry->guid)) == 0)
        {
            // Not a valid partition
            continue;
        }
 
        // We have the entry, lets create a virtual disk
        res = disk_create_new(PEACHOS_DISK_TYPE_PARTITION, entry->starting_lba, entry->ending_lba, gpt_primary_disk->sector_size, NULL);
        if (res < 0)
        {
            goto out;
        }
        // WARNING: CODE WONT COMPILE YET, UNTIL WE IMPLEMENT VIRTUAL DISKS
        // IN THE DISK FUNCTIONALITY 
        // We have mounted the partition as a virtual disk.

    }
out:
    return res;
}

int gpt_init()
{
    int res = 0;
    struct gpt_partition_table_header partition_header = {0};
    gpt_primary_disk = disk_get(0);

    if (!gpt_primary_disk)
    {
        res = -EINVARG;
        goto out;
    }

    res = gpt_partition_table_header_read(&partition_header);
    if (res < 0)
    {
        goto out;
    }

    if (memcmp(partition_header.signature, GPT_SIGNATURE, sizeof(partition_header.signature)) != 0)
    {
        // Not a gpt formatted disk
        res = -EINFORMAT;
        goto out;
    }


    // This is a GPT disk mount all partitions as seperate
    // virtual disks
    res = gpt_mount_partitions(&partition_header);
    if (res < 0)
    {
        goto out;
    }
    

out:    
    return res;
}