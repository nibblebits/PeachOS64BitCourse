#ifndef TASKSWITCHSEGMENT_H
#define TASKSWITCHSEGMENT_H

#include <stdint.h>

struct tss {
    uint16_t link;            // Previous Task Link (2 bytes)
    uint16_t reserved0;       // Reserved (2 bytes)

    uint64_t rsp0;            // Stack pointer for Ring 0
    uint64_t rsp1;            // Stack pointer for Ring 1
    uint64_t rsp2;            // Stack pointer for Ring 2

    uint64_t reserved1;       // Reserved (8 bytes)

    uint64_t ist1;            // Interrupt Stack Table 1
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;

    uint64_t reserved2;       // Reserved (8 bytes)

    uint16_t reserved3;       // Reserved (2 bytes)
    uint16_t iopb_offset;     // I/O Map Base (2 bytes)
} __attribute__((packed));

void tss_load(int tss_segment);
#endif