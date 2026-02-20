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

#include "disk.h"
#include "io/io.h"
#include "config.h"
#include "status.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "string/string.h"
#include "lib/vector/vector.h"

struct vector* disk_vector = NULL;

// A pointer to the primary hard disk
// allowing IO directly to the disk from LBA zero onwards
struct disk* disk = NULL;

// A pointer to the virtual disk that contains the primary kernel filesystem
// where kernel files are found.
struct disk* primary_fs_disk = NULL;

int disk_read_sector(int lba, int total, void* buf)
{
    // Wait for the disk not to be busy
    while(insb(0x1F7) & 0x80)
    {
        // spin
    }

    // Select drive: bits 7..4=0xE, bits 3..0 = high nibble of lba
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    // Sector count
    outb(0x1F2, (unsigned char) total);
    // LBA low, mid, high
    outb(0x1F3, (unsigned char)(lba & 0xff));
    outb(0x1F4, (unsigned char)((lba >> 8) & 0xff));
    outb(0x1F5, (unsigned char)((lba >> 16) & 0xff));

    // Read SECTORS command (0x20)
    outb(0x1F7, 0x20);

    unsigned short* ptr = (unsigned short*) buf;
    for (int b = 0; b < total; b++)
    {
        // Wait for the disk not to be busy
        while(insb(0x1F7) & 0x80)
        {
            // spin
        }

        // Check error bit
        char status = insb(0x1F7);
        if (status & 0x01)
        {
            return -EIO;
        }

        // Wait for the buffer to be ready
        while(!(insb(0x1F7) & 0x08))
        {
            // spin
        }

        // Copy from hard disk to memory
        for (int word = 0; word < 256; word++)
        {
            *ptr++ = insw(0x1F0);
        }

    }
    return 0;
}

int disk_create_new(int type, int starting_lba, int ending_lba, size_t sector_size, struct disk** disk_out)
{
    int res = 0;
    struct disk* disk = kzalloc(sizeof(struct disk));
    if (!disk)
    {
        res = -ENOMEM;
        goto out;
    }
    disk->type = type;
    disk->id = vector_count(disk_vector);
    disk->sector_size = sector_size;
    disk->starting_lba = starting_lba;
    disk->ending_lba = ending_lba;

    // Not all disks have filesystems its not an error not to have one
    disk->filesystem = fs_resolve(disk);
    if (disk->filesystem)
    {
        char fs_name[11] = {0};
        char primary_drive_fs_name[11] = {0};
        strncpy(primary_drive_fs_name, PEACHOS_KERNEL_FILESYSTEM_NAME, strlen(PEACHOS_KERNEL_FILESYSTEM_NAME));
        // Is the disk the primary disk, lets check
        disk->filesystem->volume_name(disk->fs_private, fs_name, sizeof(fs_name));
        if (strncmp(fs_name, primary_drive_fs_name, sizeof(fs_name)) == 0)
        {
            // Set the primary filesystem disk
            primary_fs_disk = disk;
        }
    }

    if (disk_out)
    {
        *disk_out = disk;
    }
    vector_push(disk_vector, &disk);
out:
    return res;
}
void disk_search_and_init()
{
    int res = 0;
    disk_vector = vector_new(sizeof(struct disk*), 4, 0);
    if (!disk_vector)
    {
        res = -ENOMEM;
        goto out;
    }

    res = disk_create_new(PEACHOS_DISK_TYPE_REAL, 0, 0, PEACHOS_SECTOR_SIZE, &disk);
    if (res < 0)
    {
        goto out;
    }
out:
    return;
}

struct disk* disk_primary()
{
    return disk;
}

struct disk* disk_primary_fs_disk()
{
    return primary_fs_disk;
}



struct disk* disk_get(int index)
{
    size_t total_disks = vector_count(disk_vector);
    if (index >= (int) total_disks)
    {
        // out of bounds no such disk is loaded
        return NULL;
    }

    struct disk* disk = NULL;
    vector_at(disk_vector, index, &disk, sizeof(disk));
    return disk;
}

int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buf)
{
    size_t absolute_lba = idisk->starting_lba + lba;
    size_t absolute_ending_lba = absolute_lba + total;
    if (absolute_ending_lba > idisk->ending_lba)
    {
        // Is this the primary disk
        if (idisk->starting_lba != 0 && idisk->ending_lba != 0)
        {
            // Out of bounds, you cannot read over to other virtual disks
            return -EIO;
        }
    }

    return disk_read_sector(absolute_lba, total, buf);
}