#ifndef DISK_H
#define DISK_H

#include "fs/file.h"

typedef unsigned int PEACHOS_DISK_TYPE;


// Represents a real physical hard disk
#define PEACHOS_DISK_TYPE_REAL 0

// Specifies this disk represents a partion/virtual-disk
#define PEACHOS_DISK_TYPE_PARTITION 1 

#define PEACHOS_KERNEL_FILESYSTEM_NAME "PEACH      "

struct disk
{
    PEACHOS_DISK_TYPE type;
    int sector_size;

    // The id of the disk
    int id;

    struct filesystem* filesystem;

    // Set both to zero for the primary disk
    // all bounds checking is ignored if set to zero.
    size_t starting_lba;
    size_t ending_lba;

    // The private data of our filesystem
    void* fs_private;
};

int disk_create_new(int type, int starting_lba, int ending_lba, size_t sector_size, struct disk** disk_out);
void disk_search_and_init();
struct disk* disk_get(int index);
int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buf);
struct disk* disk_primary_fs_disk();
struct disk* disk_primary();

#endif