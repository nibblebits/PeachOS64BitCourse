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