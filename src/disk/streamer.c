#include "streamer.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "config.h"

#include <stdbool.h>
struct disk_stream* diskstreamer_new(int disk_id)
{
    struct disk* disk = disk_get(disk_id);
    if (!disk)
    {
        return 0;
    }

    struct disk_stream* streamer = kzalloc(sizeof(struct disk_stream));
    streamer->pos = 0;
    streamer->disk = disk;
    return streamer;
}

int diskstreamer_seek(struct disk_stream* stream, int pos)
{
    stream->pos = pos;
    return 0;
}

int diskstreamer_read(struct disk_stream* stream, void* out, int total)
{
   if (total <= 0) return -1;

   char* outc = out;
   int remaining = total;

   while(remaining > 0)
   {
        int sector = stream->pos / PEACHOS_SECTOR_SIZE;
        int offset = stream->pos % PEACHOS_SECTOR_SIZE;
        int chunk = PEACHOS_SECTOR_SIZE - offset;
        if (chunk > remaining)
        {
            chunk = remaining;
        }

        char buf[PEACHOS_SECTOR_SIZE];
        int res = disk_read_block(stream->disk, sector, 1, buf);
        if (res < 0)
        {
            return res;
        }

        memcpy(outc, buf+offset, chunk);
        outc += chunk;
        stream->pos += chunk;
        remaining -= chunk;
   }
   
   return 0;
}

void diskstreamer_close(struct disk_stream* stream)
{
    kfree(stream);
}