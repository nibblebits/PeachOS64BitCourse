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

#ifndef KERNEL_FONT_H
#define KERNEL_FONT_H

#include <stdint.h>
#include <stddef.h>
#include "graphics/graphics.h"
#include "config.h"

#define FONT_IMAGE_DRAW_SUBTRACT_FROM_INDEX 32
#define FONT_IMAGE_CHARACTER_WIDTH_PIXEL_SIZE 9
#define FONT_IMAGE_CHRACTER_HEIGHT_PIXEL_SIZE 16
#define FONT_IMAGE_CHARACTER_Y_OFFSET 4


struct font
{
    
    // Ascii table from 0 to max 255
    size_t character_count;

    // pixel array
    uint8_t* character_data;

    size_t bits_width_per_character;
    size_t bits_height_per_character;

    uint8_t subtract_from_ascii_char_index_for_drawing;

    char filename[PEACHOS_MAX_PATH];
};

int font_system_init();
int font_draw_text(struct graphics_info* graphics_info, struct font* font, int screen_x, int screen_y, const char* str, struct framebuffer_pixel font_color);
int font_draw(struct graphics_info* graphics_info, struct font* font, int screen_x, int screen_y, int character, struct framebuffer_pixel font_color);
struct font* font_create(uint8_t* character_data, size_t character_count, size_t bits_width_per_character, size_t bits_height_per_character, uint8_t subtract_from_ascii_char_index_for_drawing);
struct font* font_load(const char* filename);
struct font* font_get_loaded_font(const char* filename);
struct font* font_get_system_font();

#endif