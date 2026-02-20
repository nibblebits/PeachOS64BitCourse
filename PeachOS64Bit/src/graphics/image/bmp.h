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

#ifndef GRAPHICS_IMAGE_FORMAT_BMP_H
#define GRAPHICS_IMAGE_FORMAT_BMP_H
#include "graphics/image/image.h"
#include <stddef.h>
#include <stdint.h>

#define BIT_PER_PIXEL_MONOCHROME 1
#define BIT_PER_PIXEL_16_COLORS 4
#define BIT_PER_PIXEL_256_COLORS 8
#define BIT_PER_PIXEL_65536_COLORS 16
#define BIT_PER_PIXEL_16777216_COLORS 24

#define BMP_SIGNATURE "BM"

#define BMP_COMPRESSION_UNCOMPRESSED 0
#define BMP_COMPRESSION_RLE8 1
#define BMP_COMPRESSION_RLE4 2
#define BMP_COMPRESSION_BITFIELDS 3

struct bmp_header
{
    // signature
    char bf_type[2];
    // file size
    uint32_t bf_size;
    uint16_t bf_reserved_1;
    uint16_t bf_reserved_2;

    // offset to the pixel data
    uint32_t bf_offbits;
} __attribute__((packed));

struct bmp_image_header
{   
    uint32_t bi_size;
    int32_t bi_width;
    int32_t bi_height;
    uint16_t bi_planes;
    uint16_t bi_bits_per_pixel;
    uint32_t bi_compression;
    uint32_t bi_size_image;
    uint32_t bi_x_pixels_per_m;
    uint32_t bi_y_pixels_per_m;
    // total colors used
    uint32_t bi_colors_used_count;
    // number of important colors
    uint32_t bi_important_colors_count;
} __attribute__((packed));

// color table
struct color_table
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    // unused
    uint8_t reserved;
};

struct image_format* graphics_image_format_bmp_setup();

#endif