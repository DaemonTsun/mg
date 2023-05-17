
#pragma once

#include <vulkan/vulkan_core.h>
#include "shl/array.hpp"

#include "mg/window.hpp"

namespace mg
{
// in SDL, a window with the vulkan flag must be given as parameter,
// in GLFW, the window parameter is optional.
u32 get_vulkan_instance_extensions(array<const char*> *out, mg::window *window = nullptr);
VkResult create_vulkan_surface(mg::window *window, VkInstance instance, VkSurfaceKHR *out);
}
