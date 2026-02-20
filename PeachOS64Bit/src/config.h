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

#ifndef CONFIG_H
#define CONFIG_H

#define PEACHOS_KERNEL_LOCATION 0x100000
#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10

#define KERNEL_LONG_MODE_CODE_SELECTOR 0x18
#define KERNEL_LONG_MODE_TSS_SELECTOR 0x38

#define KERNEL_LONG_MODE_CODE_GDT_INDEX  3
#define KERNEL_LONG_MODE_DATA_GDT_INDEX 4
#define KERNEL_LONG_MODE_TSS_GDT_INDEX 7

#define TSS_DESCRIPTOR_TYPE 0x89  // 0x9 = 64-bit, bit 7 present
#define PEACHOS_TOTAL_INTERRUPTS 512


// 16 Bit number defining the total number of loaded
// entries. 
#define PEACHOS_MEMORY_MAP_TOTAL_ENTRIES_LOCATION 0x210000

// Where to find the E820 records
#define PEACHOS_MEMORY_MAP_LOCATION 0x210008

// 100MB heap size
#define PEACHOS_HEAP_MINIMUM_SIZE_BYTES 104857600
#define PEACHOS_HEAP_BLOCK_SIZE 4096

// The minimal address the heap can point at, ensuring
// that the kernel does not get overwritten
#define PEACHOS_MINIMAL_HEAP_ADDRESS 0x01100000

// This is a minimal heap table starting address
#define PEACHOS_MINIMAL_HEAP_TABLE_ADDRESS 0x01000000

#define PEACHOS_MINIMAL_HEAP_TABLE_SIZE PEACHOS_MINIMAL_HEAP_ADDRESS-PEACHOS_MINIMAL_HEAP_TABLE_ADDRESS


#define PEACHOS_SECTOR_SIZE 512

#define PEACHOS_MAX_FILESYSTEMS 12
#define PEACHOS_MAX_FILE_DESCRIPTORS 512

#define PEACHOS_MAX_PATH 108

#define PEACHOS_TOTAL_GDT_SEGMENTS 6

#define PEACHOS_PROGRAM_VIRTUAL_ADDRESS 0x400000
#define PEACHOS_USER_PROGRAM_STACK_SIZE 1024 * 16
#define PEACHOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START 0x3FF000
#define PEACHOS_PROGRAM_VIRTUAL_STACK_ADDRESS_END PEACHOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START - PEACHOS_USER_PROGRAM_STACK_SIZE

#define PEACHOS_MAX_PROGRAM_ALLOCATIONS 1024
#define PEACHOS_MAX_PROCESSES 12

#define USER_DATA_SEGMENT 0x33 // Also includes requested privilage level 3 
#define USER_CODE_SEGMENT 0x2B // Also includes RPL3

#define PEACHOS_MAX_ISR80H_COMMANDS 1024

#define PEACHOS_KEYBOARD_BUFFER_SIZE 1024

#define WINDOW_MAX_TITLE 128
#endif