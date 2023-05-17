
#include <assert.h>

#include "mg/impl/window.hpp"

#if defined MG_USE_SDL
#include <SDL2/SDL_vulkan.h>

#elif defined MG_USE_GLFW
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#endif

u32 mg::get_vulkan_instance_extensions(array<const char*> *out, mg::window *window)
{
    assert(out != nullptr);
    u32 ext_count = 0;

#if defined MG_USE_SDL
    assert(window != nullptr);
    SDL_Vulkan_GetInstanceExtensions(window, &ext_count, nullptr);
    ::resize(out, ext_count);
    SDL_Vulkan_GetInstanceExtensions(window, &ext_count, out->data);

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
    if (SDL_Vulkan_CreateSurface(window, instance, out) != SDL_TRUE)
        return VK_ERROR_UNKNOWN;

    return VK_SUCCESS;
    
#elif defined MG_USE_GLFW
    return glfwCreateWindowSurface(instance, window, nullptr, out);

#endif

    return VK_ERROR_UNKNOWN;
}
