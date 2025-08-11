#include "peachos.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "file.h"
int main(int argc, char** argv)
{
    int fd = fopen("@:/blank.elf", "r");
    if (fd > 0)
    {
        struct file_stat file_stat = {0};
       
        printf("File blank.elf opened\n");
        fstat(fd, &file_stat);
        printf("File size: %i\n", file_stat.filesize);
        fclose(fd);
    }
    else
    {
        printf("File blank.elf opened failed\n");
    }

    while(1) {}

    return 0;
}