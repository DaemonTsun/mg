
#include <assert.h>
#include <string.h>

#include "shl/time.hpp"

#if defined MG_USE_SDL
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL.h>

#elif defined MG_USE_GLFW
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#endif

#include "mg/context.hpp"
#include "mg/ui.hpp"
#include "mg/impl/window.hpp"

#include <stdio.h>

#if defined MG_USE_GLFW
void _glfw_window_resize_callback(mg::window_handle *handle, int width, int height)
{
    void *ptr = glfwGetWindowUserPointer(handle);
    mg::window *window = (mg::window *)ptr;

    window->config->resize.resizing = true;
    window->config->resize.time_left = window->window_resize_timeout;
    window->config->resize.width = width;
    window->config->resize.height = width;
}
#endif

void mg::create_window(mg::window *out, const char *title, int width, int height)
{
    mg::init_windowing_system();

    mg::window_handle *handle = mg::create_window_handle(title, width, height);
    assert(handle != nullptr);

    out->handle = handle;
    out->context = mg::create_context();
    out->config = ::allocate_memory<mg::window_config>();
    memset(out->config, 0, sizeof(mg::window_config));
    ::init(&out->config->callbacks.window_event_handlers);

    out->target_fps = 60;
    out->window_resize_timeout = 0.1;
    
    mg::setup_vulkan_instance(out->context, out);
    mg::create_vulkan_surface(out->context, out);
    mg::setup_window_context(out->context, out);

#if defined MG_USE_GLFW
    glfwSetWindowUserPointer(handle, out);
    glfwSetWindowSizeCallback(handle, ::_glfw_window_resize_callback);
#endif

    ui::init(out);
    ui::set_window_ui_callbacks(out);
}

void mg::close_window(mg::window *window)
{
    assert(window != nullptr);

#if defined MG_USE_SDL
    SDL_Event e;
    e.quit.type = SDL_QUIT;
    e.quit.timestamp = SDL_GetTicks();

    SDL_PushEvent(&e);
#elif defined MG_USE_GLFW
    glfwSetWindowShouldClose(window->handle, GLFW_TRUE);
#endif
}

void mg::destroy_window(mg::window *window)
{
    assert(window != nullptr);

    ::free(&window->config->callbacks.window_event_handlers);
    ::free_memory(window->config);
    window->config = nullptr;

    ui::exit(window);
    mg::destroy_context(window->context);
    window->context = nullptr;

    mg::destroy_window_handle(window->handle);

    mg::quit_windowing_system();
}

void mg::get_window_size(mg::window *window, int *width, int *height)
{
    assert(window != nullptr);
    assert(window->handle != nullptr);
    
#if defined MG_USE_SDL
    SDL_GetWindowSize(window->handle, width, height);
    
#elif defined MG_USE_GLFW
    glfwGetWindowSize(window->handle, width, height);
#endif
}

void mg::set_window_size(mg::window *window, int width, int height)
{
    assert(window != nullptr);
    assert(window->handle != nullptr);
    
#if defined MG_USE_SDL
    SDL_SetWindowSize(window->handle, width, height);
    
#elif defined MG_USE_GLFW
    glfwSetWindowSize(window->handle, width, height);
#endif
}

void mg::default_render_function(mg::window *window, double dt)
{
    // e.g. if window size changed, exit
    if (!mg::start_rendering(window->context))
        return;

    ui::render(window);
    mg::end_rendering(window->context);

    mg::present(window->context);
}

void _update_window_resizing_timeout(mg::window *window, double dt)
{
    mg::window_config *conf = window->config;

    if (!conf->resize.resizing)
        return;

    conf->resize.time_left -= dt;
    
    if (conf->resize.time_left < 0)
    {
        conf->resize.resizing = false;

        mg::set_render_size(window->context, conf->resize.width, conf->resize.height);

        if (conf->callbacks.window_resize != nullptr)
            conf->callbacks.window_resize(window, conf->resize.width,
                                                  conf->resize.height);
    }
}

void mg::event_loop(mg::window *window, mg::event_loop_update_callback update,
                                        mg::event_loop_render_callback render)
{
    double dt;
    timespan start;
    timespan now;

    get_time(&start);
    now = start;

    bool quit = false;

    while (!quit)
    {
        double fpss = 1.0 / window->target_fps;

        get_time(&now);
        dt = get_seconds_difference(&start, &now);

        mg::poll_events(window, &quit);

        if (quit)
            break;
        
        if (dt >= fpss)
        {
            start = now;
            ::_update_window_resizing_timeout(window, dt);

            if (!window->config->resize.resizing)
            {
                mg::update(window->context, dt);
                update(window, dt);
                render(window, dt);
            }
        }
        else
        {
            double time_left_until_frame = fpss - dt;
            
            if (time_left_until_frame >= 0.01)
                sleep(time_left_until_frame);
            else
                sleep(0.0);
        }
    }
}
