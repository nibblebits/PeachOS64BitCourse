#ifndef TASKSWITCHSEGMENT_H
#define TASKSWITCHSEGMENT_H

#include <stdint.h>
struct tss
{
   uint16_t link; // Previous task link 2 bytes
   uint16_t reserved0; // Reserved (2 bytes)
   uint64_t rsp0; // Stack pointer for the ring 0
   uint64_t rsp1; // Stack pointer for ring 1
   uint64_t rsp2; // Stack pointer for ring 2

   uint64_t reserved1; // reserved 8 bytes

   uint64_t ist1; // Interurpt stack table 1
   uint64_t ist2; // stack table 2..
   uint64_t ist3; //ect...
   uint64_t ist4;
   uint64_t ist5;
   uint64_t ist6;
   uint64_t ist7;

   uint64_t reserved2; // reserved 8 bytes
   uint16_t reserved3; // reserved 2 bytes
   uint16_t iopb_offset; // i/o.. (2 bytes)
   
} __attribute__((packed));

void tss_load(int tss_segment);
#endif