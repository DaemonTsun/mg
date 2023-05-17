
#pragma once

// defines the window type depending on the windowing library used

#if defined MG_USE_SDL
struct SDL_Window;

namespace mg
{
    typedef SDL_Window window;
}

#elif defined MG_USE_GLFW
struct GLFWwindow;

namespace mg
{
    typedef GLFWwindow window;
}
#else
#error "no windowing library used"
#endif


// defined in impl/window.cpp
namespace mg
{
// init is called automatically
void init_windowing_system();
void quit_windowing_system();

mg::window *create_window(const char *title, int width, int height);
void destroy_window(mg::window *window);
void get_window_size(mg::window *window, int *width, int *height);

// events
typedef void (*window_resize_callback)(mg::window *, int width, int height);

void set_window_resize_callback(mg::window *window, mg::window_resize_callback callback);
void set_window_resize_timeout(double seconds);

// only used in SDL, parameter is SDL_Event*
// called at any event
typedef void (*window_event_handler)(mg::window *, void *);
void add_window_event_handler(mg::window_event_handler handler);
void remove_window_event_handler(mg::window_event_handler handler);

typedef void (*update_callback)(double dt);
typedef void (*render_callback)(double dt);

void set_event_loop_update_callback(mg::update_callback callback);
void set_event_loop_render_callback(mg::render_callback callback);


void poll_events(mg::window *window, bool *quit);
void event_loop(mg::window *window);
void set_target_fps(double fps);
double get_target_fps();
}
