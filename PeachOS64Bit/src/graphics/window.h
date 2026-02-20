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

#ifndef GRAPHICS_WINDOW_H
#define GRAPHICS_WINDOW_H
#include "graphics/terminal.h"
#include "graphics/graphics.h"
#include "config.h"
#include <stddef.h>
#include <stdint.h>

enum
{
    WINDOW_EVENT_TYPE_NULL,
    WINDOW_EVENT_TYPE_FOCUS,
    WINDOW_EVENT_TYPE_LOST_FOCUS,
    WINDOW_EVENT_TYPE_MOUSE_MOVE,
    WINDOW_EVENT_TYPE_MOUSE_CLICK,
    WINDOW_EVENT_TYPE_WINDOW_CLOSE,
    WINDOW_EVENT_TYPE_KEY_PRESS
};

struct window_event
{
    int type;
    int win_id;
    struct window* window;
    union
    {
        struct
        {
            // no extra data for focus event
        } focus;

        struct
        {
            int x; 
            int y;
        } move;

        struct
        {
            int x;
            int y;
        } click;

        struct
        {
            int key;
        } keypress;
    } data;
};
typedef int (*WINDOW_EVENT_HANDLER)(struct window* window, struct window_event* event);

enum
{
    WINDOW_FLAG_BORDERLESS = 0b00000001,
    WINDOW_FLAG_CLICK_THROUGH = 0b00000010,
    WINDOW_FLAG_BACKGROUND_TRANSPARENT = 0b00000100
};
struct window
{
    int id;
    struct terminal* title_bar_terminal;
    // Terminal for the body of the window
    struct terminal* terminal;

    // Contains subgraphics for titlebar window and borders
    struct graphics_info* root_graphics;

    struct graphics_info* title_bar_graphics;

    struct graphics_info* graphics;

    struct
    {
        struct
        {
            size_t x;
            size_t y ;
            size_t width;
            size_t height;
        } close_btn;
    } title_bar_components;

    struct 
    {
        // vector of WINDOW_EVENT_HANDLER*
        struct vector* handlers;
    } event_handlers;
    
    size_t width;
    size_t height;

    size_t x;
    size_t y;

    // Determines which window is drawn first 
    size_t zindex;

    // Window title
    char title[WINDOW_MAX_TITLE];
    int flags;
    
};
#endif