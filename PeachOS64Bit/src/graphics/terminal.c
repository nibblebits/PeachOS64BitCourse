#include "graphics/terminal.h"
#include "memory/heap/kheap.h"
#include "graphics/font.h"
#include "graphics/graphics.h"
#include "graphics/image/image.h"
#include "lib/vector/vector.h"
#include "memory/memory.h"
#include "status.h"
#include <stdbool.h>

struct vector* terminal_vector = NULL;
inline static size_t terminal_abs_x_for_next_character(struct terminal* terminal)
{
    return terminal->bounds.abs_x + (terminal->text.col * terminal->font->bits_width_per_character);
}

inline static size_t terminal_abs_y_for_next_character(struct terminal* terminal)
{
    return terminal->bounds.abs_y + (terminal->text.row * terminal->font->bits_height_per_chracter);
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