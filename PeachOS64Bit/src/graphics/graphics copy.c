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
void graphics_info_children_free(struct graphics_info* graphics_info);

bool graphics_bounds_check(struct graphics_info* graphics_info, int x, int y)
{
    return !(x < 0 || y < 0 || x >= graphics_info->width || y >= graphics_info->height);
}

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

void graphics_info_free(struct graphics_info* graphics_in)
{
    if (!graphics_in)
    {
        return;
    }

    if (graphics_in->children)
    {
        graphics_info_children_free(graphics_in);
    }

    if (graphics_in->pixels)
    {
        kfree(graphics_in->pixels);
        graphics_in->pixels = NULL;
    }

    if (graphics_in->framebuffer && (graphics_in->flags & GRAPHICS_FLAG_CLONED_FRAMEBUFFER))
    {
        kfree(graphics_in->framebuffer);
        graphics_in->framebuffer = NULL;
    }

    if (graphics_in->parent)
    {
        vector_pop_element(graphics_in->parent->children, &graphics_in, sizeof(struct graphics_info*));
    }

    // We dont want to free the graphics we didnt create
    if (graphics_in == loaded_graphics_info)
    {
        return;
    }

    kfree(graphics_in);
}
void graphics_info_children_free(struct graphics_info* graphics_info)
{
    if (!graphics_info)
    {
        return;
    }

    size_t total_children = vector_count(graphics_info->children);
    for(size_t i = 0; i < total_children; i++)
    {
        struct graphics_info* child = NULL;
        int res = 0;
        res = vector_at(graphics_info->children, i, &child, sizeof(struct graphics_info*));
        if (res < 0)
        {
            continue;
        }

        graphics_info_free(child);
    }

}


int graphics_pixel_get(struct graphics_info* graphics_info, uint32_t x, uint32_t y, struct framebuffer_pixel* pixel_out)
{
    int res = 0;
    if (!graphics_bounds_check(graphics_info, x, y))
    {
        res = -EINVARG;
        goto out;
    }

    *pixel_out = graphics_info->pixels[y * graphics_info->width + x];
out:
    return res;
}

int graphics_reorder(void* first_element, void* second_element)
{
    struct graphics_info* first_graphics = *((struct graphics_info**) first_element);
    struct graphics_info* second_graphics = *((struct graphics_info**) second_element);
    return first_graphics->z_index > second_graphics->z_index;
}
void graphics_set_z_index(struct graphics_info* graphics_info, uint32_t z_index)
{
    graphics_info->z_index = z_index;
    if (graphics_info->parent && graphics_info->parent->children)
    {
        vector_reorder(graphics_info->parent->children, graphics_reorder);
    }
}
void graphics_paste_pixels_to_pixels(
    struct graphics_info *graphics_info_in,
    struct graphics_info *graphics_info_out,
    uint32_t src_x,
    uint32_t src_y,
    uint32_t width,
    uint32_t height,
    uint32_t dst_x,
    uint32_t dst_y,
    int flags)
{
    uint32_t src_x_end = src_x + width;
    uint32_t src_y_end = src_y + height;

    if (src_x_end > graphics_info_in->width)
        src_x_end = graphics_info_in->width;
    if (src_y_end > graphics_info_in->height)
        src_y_end = graphics_info_in->height;

    uint32_t final_w = src_x_end - src_x;
    uint32_t final_h = src_y_end - src_y;

    bool has_transparency_key = false;
    struct framebuffer_pixel ignore_transparent_pixel = {0};
    if (memcmp(&graphics_info_out->transparency_key,
               &ignore_transparent_pixel,
               sizeof(graphics_info_out->transparency_key)) != 0)
    {
        has_transparency_key = true;
    }

    for (uint32_t lx = 0; lx < final_w; lx++)
    {
        for(uint32_t ly = 0; ly < final_h; ly++)
        {
            struct framebuffer_pixel pixel = {0};
            graphics_pixel_get(graphics_info_in, src_x+lx, src_y+ly, &pixel);

            uint32_t dx = dst_x + lx;
            uint32_t dy = dst_y + ly;

            if (dx < graphics_info_out->width && dy < graphics_info_out->height)
            {
                if (flags & GRAPHICS_FLAG_DO_NOT_OVERWRITE_TRASPARENT_PIXELS)
                {
                    if (has_transparency_key)
                    {
                        struct framebuffer_pixel* existing_pixel = &graphics_info_out->pixels[dy * graphics_info_out->width + dx];
                        if (memcmp(existing_pixel, &graphics_info_out->transparency_key, sizeof(struct framebuffer_pixel)) == 0)
                        {
                            continue;
                        }
                    }
                }
            }
            graphics_info_out->pixels[dy * graphics_info_out->width + dx] = pixel;
        }
    }
}

struct graphics_info* graphics_info_create_relative(struct graphics_info* source_graphics, size_t x, size_t y, size_t width, size_t height, int flags)
{
    int res = 0;
    struct graphics_info* new_graphics = NULL;
    if (source_graphics == NULL)
    {
        panic("Source graphics null\n");
        res = -EINVARG;
        goto out;
    }

    new_graphics = kzalloc(sizeof(struct graphics_info));
    if (!new_graphics)
    {
        res = -ENOMEM;
        goto out;
    }

    size_t parent_x = source_graphics->starting_x;
    size_t parent_y = source_graphics->starting_y;
    size_t parent_width = source_graphics->horizontal_resolution;
    size_t parent_height = source_graphics->vertical_resolution;


    // This is relative positioning
    // so we need to calculate an absolute position
    size_t starting_x = parent_x + x;
    size_t starting_y = parent_y + y;
    size_t ending_x = starting_x + width;
    size_t ending_y = starting_y + height;

    if (!(flags & GRAPHICS_FLAG_ALLOW_OUT_OF_BOUNDS))
    {
        // So we will never allow the new graphics to exceed
        // the parent rectangle
        if (ending_x > parent_x + parent_width || 
            ending_y > parent_y + parent_height)
        {
            res = -EINVARG;
            goto out;
        }
    }

    new_graphics->horizontal_resolution = source_graphics->horizontal_resolution;
    new_graphics->vertical_resolution = source_graphics->vertical_resolution;
    new_graphics->pixels_per_scanline = source_graphics->pixels_per_scanline;
    new_graphics->width = width;
    new_graphics->height = height;
    new_graphics->starting_x = starting_x;
    new_graphics->starting_y = starting_y;
    new_graphics->relative_x = x;
    new_graphics->relative_y = y;
    new_graphics->framebuffer = source_graphics->framebuffer;
    new_graphics->parent = source_graphics;
    new_graphics->children = vector_new(sizeof(struct graphics_info*), 4, 0);
    new_graphics->pixels = kzalloc(new_graphics->width * new_graphics->height * sizeof(struct framebuffer_pixel));
    if (!new_graphics->pixels)
    {
        res = -ENOMEM;
        goto out;
    }

    if (!(flags & GRAPHICS_FLAG_DO_NOT_COPY_PIXELS))
    {
        graphics_paste_pixels_to_pixels(source_graphics, new_graphics, x, y, width, height, 0, 0, GRAPHICS_FLAG_DO_NOT_OVERWRITE_TRASPARENT_PIXELS);
    }

    // We want to remove the framebuffer on delete flag
    // just in case to avoid double free from the parent who shares
    // the same framebuffer
    new_graphics->flags &= ~GRAPHICS_FLAG_CLONED_FRAMEBUFFER;

    // Add the child to the parent graphics
    vector_push(new_graphics->parent->children, &new_graphics);
    size_t total_children = vector_count(new_graphics->parent->children);
    graphics_set_z_index(new_graphics, total_children);
out:
    if (res < 0)
    {
        if (new_graphics)
        {
            if (new_graphics->children)
            {
                vector_free(new_graphics->children);
                new_graphics->children = NULL;
            }
            kfree(new_graphics);
            new_graphics = NULL;
        }

    }
    return new_graphics;
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

    // Redraw all the graphics
    graphics_redraw_all();
}
