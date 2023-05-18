
#pragma once

#include <vulkan/vulkan_core.h>
#include "shl/array.hpp"

#include "mg/window.hpp"

namespace mg
{
typedef void (*window_resize_callback)(mg::window *, int width, int height);
// only used in SDL, parameter is SDL_Event*
typedef void (*window_event_handler)(mg::window *, void *);

struct window_config
{
    struct _resize
    {
        double timeout;
        double time_left;
        bool resizing;

        int width;
        int height;
    } resize;

    struct _callbacks
    {
        mg::window_resize_callback window_resize;
        array<mg::window_event_handler> window_event_handlers;
    } callbacks;
};

// in SDL, a window with the vulkan flag must be given as parameter,
// in GLFW, the window parameter is optional.
u32 get_vulkan_instance_extensions(array<const char*> *out, mg::window *window = nullptr);
VkResult create_vulkan_surface(mg::window *window, VkInstance instance, VkSurfaceKHR *out);

// flags is only used in SDL
void init_windowing_system(u32 flags = 0);
void quit_windowing_system();

// events

void add_window_event_handler(mg::window *window, mg::window_event_handler handler);
void remove_window_event_handler(mg::window *window, mg::window_event_handler handler);

void poll_events(mg::window *window, bool *quit);

mg::window_handle *create_window_handle(const char *title, int width, int height);
void destroy_window_handle(mg::window_handle *handle);
}
