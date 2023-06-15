
#include <assert.h>

#include "shl/error.hpp"

#include "mg/ui.hpp"
#include "mg/impl/window.hpp"

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

#if defined MG_USE_SDL
#elif defined MG_USE_GLFW
#endif

void _init_quit_windowing_system(bool do_init, u32 flags = 0)
{
    static bool initialized = false;

    if (do_init)
    {
        if (initialized)
            return;

        initialized = true;

#if defined MG_USE_SDL
        if (flags == 0)
            flags = SDL_INIT_VIDEO
                  | SDL_INIT_JOYSTICK
                  | SDL_INIT_GAMECONTROLLER
                  | SDL_INIT_EVENTS;

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
    else
    {
        // quit windowing system
        if (!initialized)
            return;

        initialized = false;

#if defined MG_USE_SDL
        SDL_Quit();

#elif defined MG_USE_GLFW
        glfwTerminate();
#endif
    }
}

void mg::init_windowing_system(u32 flags)
{
    ::_init_quit_windowing_system(true, flags);
}

void mg::quit_windowing_system()
{
    ::_init_quit_windowing_system(false);
}

mg::window_handle *mg::create_window_handle(const char *title, int width, int height)
{
    mg::init_windowing_system();

    mg::window_handle *handle = nullptr;
    
#if defined MG_USE_SDL
    handle = SDL_CreateWindow(title,
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              width, height,
                              SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    
#elif defined MG_USE_GLFW
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    handle = glfwCreateWindow(width, height, title, nullptr, nullptr);
#endif

    return handle;
}

void mg::destroy_window_handle(mg::window_handle *handle)
{
    assert(handle != nullptr);

#if defined MG_USE_SDL
    SDL_DestroyWindow(handle);
    
#elif defined MG_USE_GLFW
    glfwDestroyWindow(handle);
#endif
}

// events
void mg::add_window_event_handler(mg::window *window, mg::window_event_handler handler)
{
    assert(window != nullptr);

    ::add_at_end(&window->config->callbacks.window_event_handlers, handler);
}

void mg::remove_window_event_handler(mg::window *window, mg::window_event_handler handler)
{
    u64 i = 0;
    auto *handlers = &window->config->callbacks.window_event_handlers;

    while (i < handlers->size)
    {
        if (handlers->data[i] == handler)
            ::remove_elements(handlers, i, 1);
        else
            i++;
    }
}

#if defined MG_USE_SDL
void _handle_SDL_event(mg::window *window, SDL_Event *e)
{
    mg::window_config *conf = window->config;

    switch (e->type)
    {
    case SDL_WINDOWEVENT:
        if (e->window.event == SDL_WINDOWEVENT_RESIZED)
        if (conf->callbacks.window_resize != nullptr)
        {
            conf->callbacks.window_resize(window, e->window.data1, e->window.data2);
        }

        break;

        // TODO: other window events

    default:
        break;
    }

    for_array(handler, &conf->callbacks.window_event_handlers)
        (*handler)(window, reinterpret_cast<void*>(e));
}

void _poll_SDL_events(mg::window *window, bool *quit)
{
    SDL_Event e;
    mg::window_config *conf = window->config;

    while (SDL_PollEvent(&e) != 0 && !*quit)
    switch (e.type)
    {
    case SDL_QUIT:
        *quit = true;
        return;

    case SDL_WINDOWEVENT:
    {
        if (e.window.event == SDL_WINDOWEVENT_RESIZED)
        {
            conf->resize.resizing = true;
            conf->resize.time_left = conf->resize.timeout;
            conf->resize.width = e.window.data1;
            conf->resize.height = e.window.data2;
        }
        else
            ::_handle_SDL_event(window, &e);
        break;
    }

    default:
        ::_handle_SDL_event(window, &e);
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

    *quit = glfwWindowShouldClose(window->handle);
#endif
}

u32 mg::get_vulkan_instance_extensions(array<const char*> *out, mg::window *window)
{
    assert(out != nullptr);
    u32 ext_count = 0;

#if defined MG_USE_SDL
    assert(window != nullptr);
    SDL_Vulkan_GetInstanceExtensions(window->handle, &ext_count, nullptr);
    ::resize(out, ext_count);
    SDL_Vulkan_GetInstanceExtensions(window->handle, &ext_count, out->data);

#elif defined MG_USE_GLFW
    const char **extensions = glfwGetRequiredInstanceExtensions(&ext_count);
    ::resize(out, ext_count);

    for_array(i, v, out)
        *v = extensions[i];
#endif

    return ext_count;
}

VkResult mg::create_vulkan_surface(mg::window *window, VkInstance instance, VkSurfaceKHR *out)
{
    assert(window != nullptr);
    assert(out != nullptr);

#if defined MG_USE_SDL
    if (SDL_Vulkan_CreateSurface(window->handle, instance, out) != SDL_TRUE)
        return VK_ERROR_UNKNOWN;

    return VK_SUCCESS;
    
#elif defined MG_USE_GLFW
    return glfwCreateWindowSurface(instance, window->handle, nullptr, out);

#endif

    return VK_ERROR_UNKNOWN;
}
