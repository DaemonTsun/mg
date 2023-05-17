
#include <assert.h>
#include <string.h>

#include "shl/memory.hpp"
#include "shl/array.hpp"
#include "shl/time.hpp"
#include "shl/error.hpp"

#if defined MG_USE_SDL
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL.h>

#define throw_sdlerror() \
    throw error{SDL_GetError()}

#elif defined MG_USE_GLFW
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#endif

#include "mg/window.hpp"

#if defined MG_USE_SDL
#elif defined MG_USE_GLFW
#endif

struct _window_resize
{
    double timeout;
    double t;
    bool resizing;

    int width;
    int height;
};

void _default_update_render_callback(double dt)
{}

struct _callbacks
{
    mg::window_resize_callback window_resize;
    array<mg::window_event_handler> window_event_handlers;
    
    mg::update_callback update;
    mg::render_callback render;
};

struct _windowing_info
{
    bool initialized;
    double target_fps;
    _window_resize window_resize;
    _callbacks callbacks;
};

// when exit is true, info gets deleted instead of created
_windowing_info *get_windowing_info(bool exit = false)
{
    static _windowing_info *info = nullptr;

    if (exit)
    {
        if (info != nullptr)
        {
            free(&info->callbacks.window_event_handlers);
            ::free_memory(info);
            info = nullptr;
        }
    }
    else
    {
        if (info == nullptr)
        {
            info = ::allocate_memory<_windowing_info>();
            memset(info, 0, sizeof(_windowing_info));

            info->target_fps = 60.0;
            info->callbacks.update = _default_update_render_callback;
            info->callbacks.render = _default_update_render_callback;

            init(&info->callbacks.window_event_handlers);
        }
    }
    
    return info;
}

void mg::init_windowing_system()
{
    auto info = ::get_windowing_info();

    if (info->initialized)
        return;

    info->initialized = true;

#if defined MG_USE_SDL
    u32 flags = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS;

    if (SDL_Init(flags) < 0)
        throw_sdlerror();

#elif defined MG_USE_GLFW
    glfwInit();

    if (!glfwVulkanSupported())
    {
        glfwTerminate();
        throw_error("could not set up glfw: vulkan not supported");
    }

#endif
}

void mg::quit_windowing_system()
{
#if defined MG_USE_SDL
    SDL_Quit();

#elif defined MG_USE_GLFW
    glfwTerminate();

#endif

    // this frees the windowing info
    get_windowing_info(true);
}

mg::window *mg::create_window(const char *title, int width, int height)
{
    mg::init_windowing_system();
    
#if defined MG_USE_SDL
    return SDL_CreateWindow(title,
                            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            width, height,
                            SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    
#elif defined MG_USE_GLFW
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    return glfwCreateWindow(width, height, title, nullptr, nullptr);

#endif
}

void mg::destroy_window(mg::window *window)
{
    assert(window != nullptr);

#if defined MG_USE_SDL
    SDL_DestroyWindow(window);
    
#elif defined MG_USE_GLFW
    glfwDestroyWindow(window);

#endif
}

void mg::get_window_size(mg::window *window, int *width, int *height)
{
    assert(window != nullptr);
    
#if defined MG_USE_SDL
    SDL_GetWindowSize(window, width, height);
    
#elif defined MG_USE_GLFW
    glfwGetWindowSize(window, width, height);

#endif
}

// events
#if defined MG_USE_GLFW
void _glfw_window_resize_callback(mg::window *window, int width, int height)
{
    auto info = get_windowing_info();
    info->window_resize.resizing = true;
    info->window_resize.t = info->window_resize.timeout;
    info->window_resize.width = width;
    info->window_resize.height = width;
    
}
#endif

void mg::set_window_resize_callback(mg::window *window, mg::window_resize_callback callback)
{
    get_windowing_info()->callbacks.window_resize = callback;

#if defined MG_USE_GLFW
    if (callback == nullptr)
        glfwSetWindowSizeCallback(window, nullptr);
    else
        glfwSetWindowSizeCallback(window, _glfw_window_resize_callback);
#endif
}

void mg::set_window_resize_timeout(double seconds)
{
    get_windowing_info()->window_resize.timeout = seconds;
}

void mg::add_window_event_handler(mg::window_event_handler handler)
{
    ::add_at_end(&get_windowing_info()->callbacks.window_event_handlers, handler);
}

void mg::remove_window_event_handler(mg::window_event_handler handler)
{
    u64 i = 0;
    auto *handlers = &get_windowing_info()->callbacks.window_event_handlers;

    while (i < handlers->size)
    {
        if (handlers->data[i] == handler)
            ::remove_elements(handlers, i, 1);
        else
            i++;
    }
}

void mg::set_event_loop_update_callback(mg::update_callback callback)
{
    get_windowing_info()->callbacks.update = callback;
}

void mg::set_event_loop_render_callback(mg::render_callback callback)
{
    get_windowing_info()->callbacks.render = callback;
}

void _update_window_resizing_timeout(_windowing_info *info, mg::window *window, double dt)
{
    if (info->callbacks.window_resize == nullptr)
        return;

    if (!info->window_resize.resizing)
        return;

    info->window_resize.t -= dt;
    
    if (info->window_resize.t < 0)
    {
        info->window_resize.resizing = false;
        info->callbacks.window_resize(window, info->window_resize.width, info->window_resize.height);
    }
}

#if defined MG_USE_SDL
void _handle_SDL_event(_windowing_info *info, mg::window *window, SDL_Event *e)
{
    switch (e->type)
    {
    case SDL_WINDOWEVENT:
        if (e->window.event == SDL_WINDOWEVENT_RESIZED)
        if (info->callbacks.window_resize != nullptr)
        {
            info->callbacks.window_resize(window, e->window.data1, e->window.data2);
        }

        break;

        // TODO: other window events

    default:
        break;
    }

    for_array(handler, &info->callbacks.window_event_handlers)
        (*handler)(window, reinterpret_cast<void*>(e));
}

void _poll_SDL_events(mg::window *window, bool *quit)
{
    SDL_Event e;
    auto info = get_windowing_info();

    while (SDL_PollEvent(&e) != 0 && !*quit)
    switch (e.type)
    {
    case SDL_QUIT:
        *quit = true;
        return;

    case SDL_WINDOWEVENT:
    {
        if (info->window_resize.timeout > 0
         && e.window.event == SDL_WINDOWEVENT_RESIZED)
        {
            info->window_resize.resizing = true;
            info->window_resize.t = info->window_resize.timeout;
            info->window_resize.width = e.window.data1;
            info->window_resize.height = e.window.data2;
        }
        else
            _handle_SDL_event(info, window, &e);
        break;
    }

    default:
        _handle_SDL_event(info, window, &e);
        break;
    }
}
#endif 

void mg::poll_events(mg::window *window, bool *quit)
{
#if defined MG_USE_SDL
    _poll_SDL_events(window, quit);

#elif defined MG_USE_GLFW
    glfwPollEvents();

    *quit = glfwWindowShouldClose(window);
#endif
}

void mg::event_loop(mg::window *window)
{
    _windowing_info *info = get_windowing_info();
    double dt = 0.0;
    timespan start;
    timespan now;

    get_time(&start);
    now = start;

    bool quit = false;

    while (!quit)
    {
        double fpss = 1.0 / info->target_fps;

        get_time(&now);
        dt = get_seconds_difference(&start, &now);

        mg::poll_events(window, &quit);

        if (quit)
            break;
        
        if (dt >= fpss)
        {
            start = now;
            ::_update_window_resizing_timeout(info, window, dt);

            if (!info->window_resize.resizing)
            {
                info->callbacks.update(dt);
                info->callbacks.render(dt);
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

void mg::set_target_fps(double fps)
{
    assert(fps > 0);
    get_windowing_info()->target_fps = fps;
}

double mg::get_target_fps()
{
    return get_windowing_info()->target_fps;
}
