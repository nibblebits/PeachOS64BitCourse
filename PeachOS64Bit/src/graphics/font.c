#include "graphics/font.h"
#include "graphics/graphics.h"
#include "memory/heap/kheap.h"
#include "lib/vector/vector.h"
#include "memory/memory.h"
#include "string/string.h"
#include "kernel.h"
#include "status.h"
#include <stdbool.h>

struct vector* loaded_fonts = NULL;
struct font* system_font = NULL;

int font_draw_from_index(struct graphics_info* graphics_info, struct font* font, int screen_x, int screen_y, int index_character, struct framebuffer_pixel font_color);
struct font* font_load_from_image(const char* filename, size_t pixel_width, size_t pixel_height, size_t y_offset_per_character);

struct font* font_get_system_font()
{
    return system_font;
}

/**
 * private function use font_load
 */
struct font* font_load_from_image(const char* filename, size_t pixel_width, size_t pixel_height, size_t y_offset_per_character)
{
    struct image* img_font = graphics_image_load(filename);
    if (!img_font)
    {
        return NULL;
    }

    size_t characters_per_row = img_font->width / pixel_width;
    size_t total_rows = img_font->height / pixel_height;
    size_t total_characters = total_rows * characters_per_row;
    if (total_characters > 255)
    {
        // MAX CAP OF 255 CHARACTERS
        total_characters = 255;
    }     

    size_t total_required_bits_for_character_set = 
        total_characters * 
        pixel_width *
        pixel_height;

    size_t total_required_bytes_for_character_set = total_required_bits_for_character_set / 8 ;
    if ((total_required_bits_for_character_set % 8) != 0)
    {
        total_required_bytes_for_character_set++;
    }

    size_t total_required_bits_per_character =
        pixel_width * pixel_height;
    
    size_t total_required_bytes_per_character = total_required_bits_per_character / 8;
    if ((total_required_bits_per_character % 8) != 0)
    {
        total_required_bytes_per_character++;
    }

    uint8_t* character_data = kzalloc(total_required_bytes_for_character_set);
    if (!character_data)
    {
        return NULL;
    }

    // We start from space
    for(int row = 0; row < total_rows; row++)
    {
        for (int col = 0; col < characters_per_row; col++)
        {
            size_t character_index = (row * characters_per_row) + col;
            size_t starting_x = col * pixel_width;
            size_t starting_y = row * pixel_height;

            for(size_t x = 0; x < pixel_width; x++)
            {
                for(size_t y = y_offset_per_character; y < pixel_height; y++)
                {
                    size_t abs_x = starting_x + x;
                    size_t abs_y = starting_y + y;
                    image_pixel_data pixel = graphics_image_get_pixel(img_font, abs_x, abs_y);
                    if (pixel.R != 0 || pixel.B != 0 || pixel.G != 0)
                    {
                        // if its not black then its a valid pixel for the character bitmap
                        size_t char_offset = character_index * total_required_bytes_per_character;
                        size_t bit_index = y * pixel_width + x;
                        size_t byte_index = char_offset + (bit_index / 8);
                        uint8_t bit_mask = 1 << (bit_index % 8);
                        character_data[byte_index] |= bit_mask;
                    }
                }
            }
        }
    }
}

struct font* font_get_loaded_font(const char* filename)
{
    struct font* font = NULL;
    size_t total_fonts = vector_count(loaded_fonts);
    for(size_t i = 0; i < total_fonts; i++)
    {
        vector_at(loaded_fonts, i, &font, sizeof(font));
        if (font)
        {
            if(strncmp(font->filename, filename, sizeof(font->filename)) == 0)
            {
                // we found the font
                return font;
            }
        }
    }

    return NULL;
}
struct font* font_load(const char* filename)
{
    struct font* loaded_font = font_get_loaded_font(filename);
    if(loaded_font)
    {
        return loaded_font;
    }

    loaded_font = font_load_from_image(filename, FONT_IMAGE_CHARACTER_WIDTH_PIXEL_SIZE, FONT_IMAGE_CHRACTER_HEIGHT_PIXEL_SIZE, FONT_IMAGE_CHARACTER_Y_OFFSET);
    if (loaded_font)
    {
        strncpy(loaded_font->filename, filename, sizeof(loaded_font->filename));
        // push it to the loaded fonts vector
        vector_push(loaded_fonts, &loaded_font);
    }

    return loaded_font;
}
struct font* font_create(uint8_t* character_data, size_t character_count, size_t bits_width_per_character, size_t bits_height_per_chracter, uint8_t subtract_from_ascii_char_index_for_drawing)
{
    struct font* font = kzalloc(sizeof(struct font));
    if (!font)
    {
        return NULL;
    }

    font->character_count = character_count;
    font->character_data = character_data;
    font->bits_width_per_character = bits_width_per_character;
    font->bits_height_per_chracter = bits_height_per_chracter;
    font->subtract_from_ascii_char_index_for_drawing = subtract_from_ascii_char_index_for_drawing;
    return font;
}

int font_draw_from_index(struct graphics_info* graphics_info, struct font* font, int screen_x, int screen_y, int index_character, struct framebuffer_pixel font_color)
{
    int res = 0;
    if (!font)
    {
        res = -EINVARG;
        goto out;
    }

    if (index_character > font->character_count)
    {
        res = 0;
        goto out;
    }

    size_t total_required_bits_per_character = 
        font->bits_width_per_character *
        font->bits_height_per_chracter;
    
    size_t total_required_bytes_per_character = total_required_bits_per_character / 8;
    if ((total_required_bits_per_character % 8) != 0)
    {
        total_required_bytes_per_character++;
    }

    size_t character_index = index_character * total_required_bytes_per_character;
    for(size_t x = 0; x < font->bits_width_per_character; x++)
    {
        for(size_t y = 0; y < font->bits_height_per_chracter; y++)
        {
            size_t char_offset = character_index;
            size_t bit_index = y * font->bits_width_per_character + x;
            size_t byte_index = char_offset + (bit_index / 8);
            if ((font->character_data[byte_index] >> (bit_index % 8)) & 0x01)
            {
                size_t abs_x = screen_x + x;
                size_t abs_y = screen_y + y;
                graphics_draw_pixel(graphics_info, abs_x, abs_y, font_color);
            }
        }
    }

    // redraw the region to the screen
    graphics_redraw_graphics_to_screen(graphics_info, screen_x, screen_y, font->bits_width_per_character, font->bits_height_per_chracter);
out:
    return res;
}

int font_draw(struct graphics_info* graphics_info, struct font* font, int screen_x, int screen_y, int character, struct framebuffer_pixel font_color)
{
    character -= (int) font->subtract_from_ascii_char_index_for_drawing;
    return font_draw_from_index(graphics_info, font, screen_x, screen_y, character, font_color);
}

int font_draw_text(struct graphics_info* graphics_info, struct font* font, int screen_x, int screen_y, const char* str, struct framebuffer_pixel font_color)
{
    int res = 0;
    int x = screen_x;
    int y = screen_y;
    while(*str != 0)
    {
        res = font_draw(graphics_info, font, x, y, *str, font_color);
        if (res < 0)
        {
            break;
        }

        x += font->bits_width_per_character;
        str++;
    }
    return res;
}
int font_system_init()
{
    int res = 0;
    loaded_fonts = vector_new(sizeof(struct font*), 4, 0);
    if (!loaded_fonts)
    {
        res = -ENOMEM;
        goto out;
    }

    system_font = font_load("@:/sysfont.bmp");
    if (!system_font)
    {
        panic("failed to load system font\n");
    }

out:
    return res;
}
