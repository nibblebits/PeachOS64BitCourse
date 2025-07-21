#include "graphics/terminal.h"
#include "memory/heap/kheap.h"
#include "graphics/font.h"
#include "graphics/graphics.h"
#include "graphics/image/image.h"
#include "lib/vector/vector.h"
#include "memory/memory.h"
#include "status.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct vector* terminal_vector = NULL;
inline static size_t terminal_abs_x_for_next_character(struct terminal* terminal)
{
    return terminal->bounds.abs_x + (terminal->text.col * terminal->font->bits_width_per_character);
}

inline static size_t terminal_abs_y_for_next_character(struct terminal* terminal)
{
    return terminal->bounds.abs_y + (terminal->text.row * terminal->font->bits_height_per_character);
}

void terminal_system_setup()
{
    terminal_vector = vector_new(sizeof(struct terminal*), 4, 0);
}

struct terminal* terminal_create(struct graphics_info* graphics_info, int starting_x, int starting_y, size_t width, size_t height, struct font* font, struct framebuffer_pixel font_color, int flags)
{
    int res = 0;
    if (font == NULL)
    {
        return NULL;
    }

    if (graphics_info == NULL)
    {
        return NULL;
    }

    if (starting_x < 0 || starting_y < 0 || 
        starting_x >= graphics_info->horizontal_resolution ||
        starting_y >= graphics_info->vertical_resolution)
    {
        return NULL;
    }

    struct terminal* terminal = kzalloc(sizeof(struct terminal));
    if (!terminal)
    {
        res = -ENOMEM;
        goto out;
    }

    terminal->graphics_info = graphics_info;
    terminal->terminal_background = NULL;
    terminal->text.row = 0;
    terminal->text.col = 0;
    terminal->bounds.abs_x = starting_x;
    terminal->bounds.abs_y = starting_y;
    terminal->bounds.width = width;
    terminal->bounds.height = height;
    terminal->font = font;
    terminal->font_color = font_color;
    terminal->flags = flags;

    // Save the background of the screen, whats behind it
    // where our terminal coords are 
    terminal_background_save(terminal);

    vector_push(terminal_vector, &terminal);

out:
    if (res < 0)
    {
        terminal_free(terminal);
        terminal = NULL;
    }

    return terminal;
}

void terminal_free(struct terminal* terminal)
{
    if (terminal->terminal_background)
    {
        kfree(terminal->terminal_background);
        terminal->terminal_background = NULL;
    }

    vector_pop_element(terminal_vector, &terminal, sizeof(terminal));
    kfree(terminal);
}

struct terminal* terminal_get_at_screen_position(size_t x, size_t y, struct terminal* ignore_terminal)
{
    int res = 0;
    struct terminal* found_terminal = NULL;
    size_t total_children = vector_count(terminal_vector);
    for(size_t i = 0; i < total_children; i++)
    {
        struct terminal* terminal = NULL;
        res = vector_at(terminal_vector, i, &terminal, sizeof(terminal));
        if (res < 0)
        {
            goto out;
        }

        if (terminal == ignore_terminal)
        {
            continue;
        }

        if (x >= terminal->bounds.abs_x &&
            x <= terminal->bounds.width && 
            y >= terminal->bounds.abs_y &&
            y <= terminal->bounds.height)
        {
            found_terminal = terminal;
            break;
        }
    }

out:
    if (res < 0)
    {
        found_terminal = NULL;
    }
    return found_terminal;
}

void terminal_background_save(struct terminal* terminal)
{
    size_t width = terminal->bounds.width;
    size_t height = terminal->bounds.height;
    size_t total_pixels = width * height;
    size_t buffer_size = total_pixels * sizeof(struct framebuffer_pixel);
    
    if (!terminal->terminal_background)
    {
        // Create a terminal background
        terminal->terminal_background = kzalloc(buffer_size);
        if (!terminal->terminal_background)
        {
            // ENOMEM
            return;
        }
    }    

    // Copy the pixels from the terminal graphics so we have a clone
    memcpy(terminal->terminal_background, terminal->graphics_info->pixels, buffer_size);

    // We now have an exact copy of the graphics info pixels of the terminal
    // at the point the background save function was called.
}

static void terminal_handle_newline(struct terminal* terminal)
{
    terminal->text.row++;
    size_t total_rows_per_term = terminal_total_rows(terminal);
    // wrap vertically if we hit the bottom 
    if (terminal->text.row >= total_rows_per_term)
    {
        terminal->text.row = 0;
    }

    // Reset the column 
    terminal->text.col = 0;
}

static void terminal_update_position_after_draw(struct terminal* terminal)
{
    terminal->text.col +=1;
    size_t total_cols_per_row = terminal_total_cols(terminal);
    size_t total_rows_per_term = terminal_total_rows(terminal);
    if (terminal->text.col >= total_cols_per_row)
    {
        terminal->text.col = 0;
        terminal->text.row++;
    }

    if (terminal->text.row >= total_rows_per_term)
    {
        terminal->text.col = 0;
        terminal->text.row = 0;
    }
}

int terminal_cursor_set(struct terminal* terminal, int row, int col)
{
    int res = 0;
    size_t total_cols_per_row = terminal_total_cols(terminal);
    size_t total_rows_per_term = terminal_total_rows(terminal);
    if (col < 0 || col >= total_cols_per_row)
    {
        res = -EINVARG;
        goto out;
    }

    if (row < 0 || row >= total_rows_per_term)
    {
        res = -EINVARG;
        goto out;
    }

    terminal->text.row = row;
    terminal->text.col = col;
out:
    return res;
}

int terminal_cursor_row(struct terminal* terminal)
{
    return terminal->text.row;
}


int terminal_cursor_col(struct terminal* terminal)
{
    return terminal->text.col;
}

int terminal_total_cols(struct terminal* terminal)
{
    return terminal->bounds.width / terminal->font->bits_width_per_character;
}

int terminal_total_rows(struct terminal* terminal)
{
    return terminal->bounds.height / terminal->font->bits_height_per_character;
}

static bool terminal_bounds_check(struct terminal* terminal, size_t abs_x, size_t abs_y)
{
    size_t starting_x = terminal->bounds.abs_x;
    size_t starting_y = terminal->bounds.abs_y;
    size_t ending_x = terminal->bounds.abs_x + terminal->bounds.width;
    size_t ending_y = terminal->bounds.abs_y + terminal->bounds.height;

    return (abs_x >= starting_x && abs_x <= ending_x && abs_y >= starting_y && abs_y <= ending_y);
}

void terminal_restore_background(struct terminal* terminal, int sx, int sy, int width, int height)
{
    for(int x = 0; x < width; x++)
    {
        for(int y = 0; y < height; y++)
        {
            size_t abs_x = terminal->bounds.abs_x + sx + x;
            size_t abs_y = terminal->bounds.abs_y + sy + y;
            // did we overflow
            if (abs_x > terminal->bounds.width || abs_y > terminal->bounds.height)
            {
                // We cant draw that just continue
                continue;
            }

            struct framebuffer_pixel background_pixel = {0};
            background_pixel = terminal->terminal_background[y * terminal->bounds.width + x];
            graphics_draw_pixel(terminal->graphics_info, abs_x, abs_y, background_pixel);

        }
    }

    size_t abs_x = terminal->bounds.abs_x + sx;
    size_t abs_y = terminal->bounds.abs_y + sy;
    graphics_redraw_graphics_to_screen(terminal->graphics_info, abs_x, abs_y, width,height);
}