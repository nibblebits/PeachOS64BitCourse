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