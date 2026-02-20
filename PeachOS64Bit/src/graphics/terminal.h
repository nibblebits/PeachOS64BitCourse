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

#ifndef GRAPHICS_TERMINAL_H
#define GRAPHICS_TERMINAL_H

#include <stdint.h>
#include <stddef.h>
#include "graphics/font.h"
#include "graphics/graphics.h"

enum
{
    TERMINAL_FLAG_BACKSPACE_ALLOWED = 0b00000001
};

struct terminal
{
    // Graphics info that the terminal is binded to
    struct graphics_info* graphics_info;

    /***
     * A clone of the original graphics_info pixels when the terminal is first created
     */
    struct framebuffer_pixel* terminal_background;
    struct
    {
        // the column and row of where the next character will be wrote
        size_t row;
        size_t col;
    } text;

    struct
    {
        size_t abs_x;
        size_t abs_y;

        size_t width;
        size_t height;
    } bounds;

    struct font* font;
    struct framebuffer_pixel font_color;
    int flags;
};

void terminal_system_setup();
void terminal_free(struct terminal* terminal);
struct terminal* terminal_create(struct graphics_info* graphics_info, int starting_x, int starting_y, size_t width, size_t height, struct font* font, struct framebuffer_pixel font_color, int flags);
void terminal_background_save(struct terminal* terminal);
struct terminal* terminal_get_at_screen_position(size_t x, size_t y, struct terminal* ignore_terminal);
int terminal_print(struct terminal* terminal, const char* message);
int terminal_draw_rect(struct terminal* terminal, uint32_t x, uint32_t y , size_t width, size_t height, struct framebuffer_pixel pixel_color);
int terminal_draw_image(struct terminal* terminal, uint32_t x, uint32_t y, struct image* img);
void terminal_ignore_color_finish(struct terminal* terminal);
void terminal_ignore_color(struct terminal* terminal, struct framebuffer_pixel pixel_color);
void terminal_transparency_key_remove(struct terminal* terminal);
void terminal_transparency_key_set(struct terminal* terminal, struct framebuffer_pixel pixel_color);
int terminal_pixel_set(struct terminal* terminal, size_t x, size_t y, struct framebuffer_pixel pixel_color);
int terminal_write(struct terminal* terminal, int c);
int terminal_backspace(struct terminal* terminal);
void terminal_restore_background(struct terminal* terminal, int sx, int sy, int width, int height);
int terminal_total_rows(struct terminal* terminal);
int terminal_total_cols(struct terminal* terminal);
int terminal_cursor_col(struct terminal* terminal);
int terminal_cursor_row(struct terminal* terminal);
int terminal_cursor_set(struct terminal* terminal, int row, int col);

#endif