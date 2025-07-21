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

#endif