
#pragma once

#include <vulkan/vulkan_core.h>

#include "shl/type_functions.hpp"
#include "shl/error.hpp"

namespace mg
{
const char *vk_result_name(VkResult res);

struct vk_error
{
    VkResult result;
    const char *what;
};
}

#define throw_vk_error(RES, FMT, ...) \
    throw mg::vk_error{RES, format_error(__FILE__ " " MACRO_TO_STRING(__LINE__) ": %s " FMT, mg::vk_result_name(RES) __VA_OPT__(,) __VA_ARGS__)}
