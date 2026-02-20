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

#include "bmp.h"
#include "image.h"
#include "memory/heap/kheap.h"
#include "string/string.h"
#include "memory/memory.h"
#include <stdint.h>
#include <stddef.h>
#include "status.h"

struct image* bmp_img_load(void* memory, size_t size)
{
    int res = 0;
    struct image* img = NULL;
    struct bmp_header* header = NULL;
    struct bmp_image_header* bmp_image_header = NULL;

    if (size < sizeof(struct bmp_header))
    {
        res = -EINFORMAT;
        goto out;
    }

    header = (struct bmp_header*) memory;
    // check for bmp signature
    if (memcmp(header->bf_type, BMP_SIGNATURE, sizeof(header->bf_type)) != 0)
    {
        // This is not a BMP file
        res = -EINFORMAT;
        goto out;
    }

    // Does image header start after bmp header
    if (header->bf_offbits >= size)
    {
        // Offset is outside of the file, how can it be a bmp file
        res = -EINFORMAT;
        goto out;
    }

    bmp_image_header = (struct bmp_image_header*)((uintptr_t) header+sizeof(struct bmp_header));

    // Allocate our image structure
    img = kzalloc(sizeof(struct image));
    if (!img)
    {
        res = -ENOMEM;
        goto out;
    }

    // Check compression
    if (bmp_image_header->bi_compression != BMP_COMPRESSION_UNCOMPRESSED)
    {
        // We only support uncompressed bmp
        res = -EUNIMP;
        goto out;
    }

    uint16_t bits_per_pixel = bmp_image_header->bi_bits_per_pixel;
    if (bits_per_pixel != 24)
    {
        // Only 24-bit bmp is supported
        res = -EUNIMP;
        goto out;
    }

    uint16_t bytes_per_pixel = bits_per_pixel / 8; // 3 for 24-bit

    // bi_height can be negative
    bool bottom_up = (bmp_image_header->bi_height > 0);
    int32_t height = bottom_up 
                     ?  bmp_image_header->bi_height
                     : -(bmp_image_header->bi_height);

    int32_t width = bmp_image_header->bi_width;
    if (width <= 0 || height <= 0)
    {
        // invlaid dimensions
        res = -EINFORMAT;
        goto out;
    }

    // prepare the image
    img->width = width;
    img->height = height;

    size_t pixel_data_size = (size_t) width * (size_t) height * sizeof(image_pixel_data);
    img->data = kzalloc(pixel_data_size);
    if (!img->data)
    {
        res = -ENOMEM;
        goto out;
    }

    // Figure out the row sizes
    size_t raw_row_size = (size_t) width * bytes_per_pixel; // width*3 for 24-bits
    size_t padded_row_size = (raw_row_size+3) & ~3;

    // Make sure we dont overflow
    if ((header->bf_offbits + (padded_row_size * (size_t) height)) > size)
    {
        res = -EINFORMAT;
        goto out;
    }

    uint8_t* bmp_first_pixel_ptr = (uint8_t*) memory+header->bf_offbits;
    // read rows
    for(int row = 0; row < height; row++)
    {
        uint8_t* row_ptr = bmp_first_pixel_ptr + (row * padded_row_size);

        // destination row
        int dest_row = bottom_up ? (height-1 -row) : row; // flip if its bottom-up
        for(int col = 0; col < width; col++)
        {
            // each pixel is 3 bytes
            uint8_t* bmp_pixel = row_ptr + (col * 3);
            uint8_t B = bmp_pixel[0];
            uint8_t G = bmp_pixel[1];
            uint8_t R = bmp_pixel[2]; 

             // write our image buffer
            size_t pixel_index = (dest_row * (size_t) width) + (size_t) col;
            image_pixel_data* out_pix = &((image_pixel_data*)img->data)[pixel_index];
            out_pix->R = R;
            out_pix->G = G;
            out_pix->B = B;
            // zero alpha.
            out_pix->A = 0;
        }
    }


out:
    if (res < 0)
    {
        if (img)
        {
            if (img->data)
                kfree(img->data);
            kfree(img);
            img = NULL;
        }
    }
    return img;
}

void bmp_img_free(struct image* image)
{
    if (!image)
        return;
    
    if(image->data)
        kfree(image->data);
    
    kfree(image);
}

int bmp_img_format_register(struct image_format* format)
{
    return 0;
}

void bmp_img_format_unregister(struct image_format* format)
{

}

struct image_format bmp_img_format = {
    .mime_type     = {"image/bmp"},
    .image_load_function = bmp_img_load,
    .image_free_function = bmp_img_free,
    .on_register_function = bmp_img_format_register,
    .on_unregister_function = bmp_img_format_unregister
};

struct image_format* graphics_image_format_bmp_setup()
{
    return &bmp_img_format;
}