#ifndef GRAPHICS_IMAGE_H
#define GRAPHICS_IMAGE_H

#include <stdint.h>
#include <stddef.h>

struct image_format;
typedef union image_pixel_data {
    uint32_t data;      // Full 32-bit data RGBA
    struct {
        uint8_t R;
        uint8_t G;
        uint8_t B;
        uint8_t A;
    };
} image_pixel_data;

struct image
{
    uint32_t width;
    uint32_t height;

    // Pixel data RGB
    image_pixel_data* data;

    // Private data of the image
    void* private;

    // A pointer to the format that loaded this image
    struct image_format* format;
};

typedef struct image* (*image_load_function)(void* memory, size_t size);
typedef void (*image_free_function)(struct image* image);
typedef int (*image_format_register_function)(struct image_format* format);
typedef void (*image_format_unregister_function)(struct image_format* format);

#define IMAGE_FORMAT_MAX_MIME_TYPE 64
struct image_format
{
    char mime_type[IMAGE_FORMAT_MAX_MIME_TYPE];
    image_load_function image_load_function;
    image_free_function image_free_function;

    image_format_register_function on_register_function;
    image_format_unregister_function on_unregister_function;

    // private data for the image format
    void* private;
};

int graphics_image_formats_load();
void graphics_image_format_unload(struct image_format* format);
void graphics_image_formats_unload();
void graphics_image_free(struct image* image);
struct image* graphics_image_load(const char* path);
image_pixel_data graphics_image_get_pixel(struct image* image, int x, int y);
struct image* graphics_image_load_from_memory(void* memory, size_t max);
struct image_format* graphics_image_format_get(const char* mime_type);
#endif