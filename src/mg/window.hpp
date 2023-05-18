
#pragma once

// defines the window handle type depending on the windowing library used

#if defined MG_USE_SDL
struct SDL_Window;

namespace mg
{
    typedef SDL_Window window_handle;
}

#elif defined MG_USE_GLFW
struct GLFWwindow;

namespace mg
{
    typedef GLFWwindow window_handle;
}
#else
#error "no windowing library used"
#endif

namespace mg
{
struct context;
struct window_config;

struct window
{
    mg::context *context;
    mg::window_handle *handle;
    mg::window_config *config;

    double target_fps;
    double window_resize_timeout;
};

void create_window(mg::window *out, const char *title, int width, int height);
void destroy_window(mg::window *window);

void get_window_size(mg::window *window, int *width, int *height);
void set_window_size(mg::window *window, int  width, int  height);

typedef void (*event_loop_update_callback)(mg::window *, double);
typedef void (*event_loop_render_callback)(mg::window *, double);

// basically just renders the UI
void default_render_function(mg::window *window, double dt);

void event_loop(mg::window *window, mg::event_loop_update_callback update
                                  , mg::event_loop_render_callback render = mg::default_render_function);
}
