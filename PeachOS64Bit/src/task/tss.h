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