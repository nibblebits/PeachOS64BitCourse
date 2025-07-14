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
    size_t bits_height_per_chracter;

    uint8_t subtract_from_ascii_char_index_for_drawing;

    char filename[PEACHOS_MAX_PATH];
};

#endif