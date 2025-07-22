#include "graphics.h"
#include "kernel.h"
#include "memory/paging/paging.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "lib/vector/vector.h"
#include "status.h"


struct graphics_info* loaded_graphics_info = NULL;

// Vector of graphics information, containing every graphics vector
// that is currently in memory
struct vector* graphics_info_vector = NULL;

void* real_framebuffer = NULL;
void* real_framebuffer_end = NULL;
size_t real_framebuffer_width = 0;
size_t real_framebuffer_height = 0;
size_t real_framebuffer_pixels_per_scanline = 0;

void graphics_redraw_children(struct graphics_info* g);

struct graphics_info* graphics_screen_info()
{
    return loaded_graphics_info;
}

void graphics_paste_pixels_to_framebuffer(
    struct graphics_info* src_info,
    uint32_t src_x,
    uint32_t src_y,
    uint32_t width,
    uint32_t height,
    uint32_t dst_abs_x, // absolute x on the screen to paste pixels
    uint32_t dst_abs_y  // absolute y on the screen to paste pixels
)
{
    if (!src_info)
    {
        return;
    }

    uint32_t src_x_end = src_x+width;
    uint32_t src_y_end = src_y+height;

    if (src_x_end > src_info->width)
        src_x_end = src_info->width;
    if (src_y_end > src_info->height)
        src_y_end = src_info->height;

    // Rectangle width/height after clipping
    uint32_t final_w = src_x_end - src_x;
    uint32_t final_h = src_y_end - src_y;

    if (final_w == 0 || final_h == 0)
        return;

    struct graphics_info* screen = graphics_screen_info();
    uint32_t screen_w = screen->horizontal_resolution;
    uint32_t screen_h = screen->vertical_resolution;

    // Is the destination off screen if so skip
    if (dst_abs_x >= screen_w || dst_abs_y >= screen_h)
        return;

    uint32_t dst_x_end = dst_abs_x + final_w;
    uint32_t dst_y_end = dst_abs_y + final_h;
    if (dst_x_end > screen_w)
    {
        dst_x_end = screen_w;
    }

    if (dst_y_end > screen_h)
    {
        dst_y_end = screen_h;
    }

    uint32_t clipped_w = dst_x_end - dst_abs_x;
    uint32_t clipped_h = dst_y_end - dst_abs_y;
    if (clipped_w == 0 || clipped_h == 0)
        return;

    // Copy line by line
    for(uint32_t ly = 0; ly < clipped_h; ly++)
    {
        for (uint32_t lx = 0; lx < clipped_w; lx++)
        {
            struct framebuffer_pixel p = src_info->pixels[(src_y + ly) * src_info->width + (src_x + lx)];

            // Transparency color, check if we have one
            // black pixel transparency color means no transparency color.
            struct framebuffer_pixel no_transparency_color = {0};

            // Do we have a transparncy key?
            if(memcmp(&src_info->transparency_key, &no_transparency_color, sizeof(no_transparency_color)) != 0)
            {
                // We have a transprancy key does it match
                if (memcmp(&p, &src_info->transparency_key, sizeof(p)) == 0)
                {
                    // Continue do not draw this pixel.
                    continue;
                }
            }

            // Write to the screen
            screen->framebuffer[(dst_abs_y + ly) * screen->pixels_per_scanline + (dst_abs_x + lx)] = p; 
        }
    }
}

void graphics_draw_pixel(struct graphics_info* graphics_info, uint32_t x, uint32_t y, struct framebuffer_pixel pixel)
{
    struct framebuffer_pixel black_pixel = {0};
    // Black pixels can be ignored
    if (memcmp(&graphics_info->ignore_color, &black_pixel, sizeof(graphics_info->ignore_color)) != 0)
    {
        if (memcmp(&graphics_info->ignore_color, &pixel, sizeof(graphics_info->ignore_color)) == 0)
        {
            // Ignore color was found, so do not draw this pixel
            return;
        }
    }

    // Transparency key is only computed during redraw
    if (x < graphics_info->width && y < graphics_info->height)
    {
        // In bounds so draw the pixel
        graphics_info->pixels[y * graphics_info->width + x] = pixel;
    }
}

void graphics_draw_image(struct graphics_info* graphics_info, struct image* image, int x, int y )
{
    if (!image)
    {
        return;
    }

    if (!graphics_info)
    {
        graphics_info = loaded_graphics_info;
    }

    // Loop through each image pixel
    for(size_t lx = 0; lx < (size_t)image->width; lx++)
    {
        for(size_t ly = 0; ly < (size_t) image->height; ly++)
        {
            size_t sx = x + lx;
            size_t sy = y + ly;

            image_pixel_data* pixel_data =
                &((image_pixel_data*)image->data)[(ly*image->width)+lx];
            
            struct framebuffer_pixel fb_pixel = {0};
            fb_pixel.red = pixel_data->R;
            fb_pixel.green = pixel_data->G;
            fb_pixel.blue = pixel_data->B;

            // Draw the pixel
            graphics_draw_pixel(graphics_info, sx, sy, fb_pixel);
        }
    }
}
void graphics_redraw_only(struct graphics_info* g)
{
    if (!g)
    {
        return;
    }

    // Copy from the local buffer to the absolute screen framebuffer
    graphics_paste_pixels_to_framebuffer(
        g,
        0,0,
        g->width, g->height, 
        g->starting_x,
        g->starting_y
    );

}

void graphics_redraw_children(struct graphics_info* g)
{
    size_t total_children = vector_count(g->children);
    for(size_t i = 0; i < total_children; i++)
    {
        struct graphics_info* child = NULL;
        vector_at(g->children, i, &child, sizeof(child));
        if (child)
        {
            graphics_redraw(child);
        }
    }
}

void graphics_redraw_region(struct graphics_info* g, uint32_t local_x , uint32_t local_y, uint32_t width, uint32_t height)
{
    if (!g)
    {
        return;
    }

    if (local_x >= g->width || local_y >= g->height)
    {
        return;
    }

    if (local_x + width > g->width)
    {
        return;
    }
    if (local_y + height > g->height)
    {
        height = g->height - local_y;
    }

    uint32_t dst_abs_x = g->starting_x + local_x;
    uint32_t dst_abs_y = g->starting_y + local_y;

    graphics_paste_pixels_to_framebuffer(
        g,
        local_x, local_y,
        width, height,
        dst_abs_x, dst_abs_y
    );
    
    uint32_t region_abs_left = dst_abs_x;
    uint32_t region_abs_top = dst_abs_y;
    uint32_t region_abs_right = dst_abs_x + width;
    uint32_t region_abs_bottom = dst_abs_y + height;

    // Now check each child, and if a child overlaps this redraw region
    // then calculate the intersection and redraw that region
    size_t child_count = vector_count(g->children);
    for(size_t i = 0; i < child_count; i++)
    {
        struct graphics_info* child = NULL;
        vector_at(g->children, i, &child, sizeof(child));
        if(!child)
        {
            continue;
        }

        // Childs absolute rectangle
        uint32_t child_abs_left = child->starting_x;
        uint32_t child_abs_top = child->starting_y;
        uint32_t child_abs_right = child->starting_x + child->width;
        uint32_t child_abs_bottom = child->starting_y + child->height;

        // Compute the intersection between the childs rectangle and the draw region
        uint32_t intersect_left = MAX(child_abs_left, region_abs_left);
        uint32_t intersect_top = MAX(child_abs_top, region_abs_top);
        uint32_t intersect_right = MAX(child_abs_right, region_abs_right);
        uint32_t intersect_bottom = MIN(child_abs_bottom, region_abs_bottom);

        if (intersect_right > intersect_left && intersect_bottom > intersect_top)
        {
            uint32_t child_local_x = intersect_left - child_abs_left;
            uint32_t child_local_y = intersect_top - child_abs_top;
            uint32_t intersect_width = intersect_right - intersect_left;
            uint32_t intersect_height = intersect_bottom - intersect_top;

            // Redraw the region
            graphics_redraw_region(child, child_local_x, child_local_y, intersect_width, intersect_height);
        }
    }
}

void graphics_ignore_color(struct graphics_info* graphics_info, struct framebuffer_pixel pixel_color)
{
    graphics_info->ignore_color = pixel_color;
}

void graphics_transparency_key_set(struct graphics_info* graphics_info, struct framebuffer_pixel pixel_color)
{
    graphics_info->ignore_color = pixel_color;
}

void graphics_transparency_key_remove(struct graphics_info* graphics_info)
{
    // black means transparency
    struct framebuffer_pixel pixel_black = {0};
    graphics_info->transparency_key = pixel_black;
}

void graphics_ignore_color_finish(struct graphics_info* graphics_info)
{
    // black means transparent
    struct framebuffer_pixel black_color = {0};
    graphics_info->ignore_color = black_color;
}


void graphics_draw_rect(
    struct graphics_info* graphics_info,
    uint32_t x,
    uint32_t y,
    size_t width,
    size_t height,
    struct framebuffer_pixel pixel_color
)
{
    uint32_t x_end = x + (uint32_t) width;
    uint32_t y_end = y + (uint32_t) height;
    for (uint32_t lx = x; lx < x_end; lx++)
    {
        for(uint32_t ly = y; ly < y_end; ly++)
        {
            graphics_draw_pixel(graphics_info, lx, ly, pixel_color);
        }
    }
}

void graphics_redraw_graphics_to_screen(struct graphics_info* relative_graphics, uint32_t rel_x, uint32_t rel_y, uint32_t width, uint32_t height)
{
    uint32_t abs_screen_x = relative_graphics->starting_x+rel_x;
    uint32_t abs_screen_y = relative_graphics->starting_y + rel_y;
    graphics_redraw_region(graphics_screen_info(), abs_screen_x, abs_screen_y, width, height);
}

void graphics_redraw(struct graphics_info* g)
{
    if (!g)
        return;
    
    graphics_redraw_only(g);

    // Redraw the children
    graphics_redraw_children(g);
}

void graphics_redraw_all()
{
    // Redraw only the source graphics, which will redraw all the children
    graphics_redraw(graphics_screen_info());
}

void graphics_setup(struct graphics_info* main_graphics_info)
{
    if (loaded_graphics_info)
    {
        panic("The graphic system was already loaded\n");
    }

    real_framebuffer = main_graphics_info->framebuffer;
    real_framebuffer_width = main_graphics_info->horizontal_resolution;
    real_framebuffer_height = main_graphics_info->vertical_resolution;
    real_framebuffer_pixels_per_scanline = main_graphics_info->pixels_per_scanline;
    size_t framebuffer_size = real_framebuffer_width*real_framebuffer_pixels_per_scanline * sizeof(struct framebuffer_pixel);
    real_framebuffer_end = (void*) ((uintptr_t) real_framebuffer+framebuffer_size);

    void* new_framebuffer_memory = kzalloc(framebuffer_size);
    main_graphics_info->framebuffer = new_framebuffer_memory;
    main_graphics_info->children = vector_new(sizeof(struct graphics_info*), 4, 0);
    main_graphics_info->pixels = kzalloc(framebuffer_size);
    main_graphics_info->width = main_graphics_info->horizontal_resolution;
    main_graphics_info->height = main_graphics_info->vertical_resolution;
    main_graphics_info->relative_x = 0;
    main_graphics_info->relative_y = 0;
    main_graphics_info->starting_x = 0;
    main_graphics_info->starting_y = 0;

    // Map the memory we allocated to point to the frame buffer point 
    paging_map_to(kernel_desc(), new_framebuffer_memory, real_framebuffer, real_framebuffer_end, PAGING_IS_WRITEABLE | PAGING_IS_PRESENT);

    loaded_graphics_info = main_graphics_info;
    for(uint32_t y = 0; y < main_graphics_info->vertical_resolution; y++)
    {
        for(uint32_t x = 0; x < main_graphics_info->horizontal_resolution; x++)
        {
            struct framebuffer_pixel pixel = {0, 0, 0, 0};
            graphics_draw_pixel(main_graphics_info, x, y, pixel);
        }
    }

    graphics_info_vector = vector_new(sizeof(struct graphics_info*), 4, 0);

    // Start by pushing the main graphics information to the vector
    vector_push(graphics_info_vector, &main_graphics_info);

    // Load the image formats.
    graphics_image_formats_init();
}
