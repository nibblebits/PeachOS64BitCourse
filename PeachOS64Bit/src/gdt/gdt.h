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

#ifndef GDT_H
#define GDT_H
#include <stdint.h>
struct gdt_entry
{
    uint16_t segment;
    uint16_t base_first;
    uint8_t base;
    uint8_t access;
    uint8_t high_flags;
    uint8_t base_24_31_bits;
} __attribute__((packed));

struct tss_desc_64
{
    // Lower 8 bytes
    uint16_t limit0;      // bits 0..15 of the limit
    uint16_t base0;       // bits 0..15 of the base
    uint8_t base1;       // Bits 16..23 of the base
    uint8_t type;        // Bits 40..47; type=0x09(available) DPL=0, P=1 = 0x89
    uint8_t limit1_flags; // bits 48..51 are limit(19,16), bits 52.55 are flags
    uint8_t base2;       // bits 24..31 of the base

    // upper 8 bytes
    uint32_t base3; // bits 32..63 of the base
    uint32_t reserved;   // bits 64.95  can be zero or hold additional data.
} __attribute__((packed));


void gdt_set(struct gdt_entry* gdt_entry, void* address, uint16_t limit_low, uint8_t access_byte, uint8_t flags);
void gdt_set_tss(struct tss_desc_64* desc, void* tss_addr, uint16_t limit, uint8_t type, uint8_t flags);

#endif