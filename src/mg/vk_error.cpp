
#include <vulkan/vk_enum_string_helper.h>

#include "mg/vk_error.hpp"

const char *mg::vk_result_name(VkResult res)
{
    return string_VkResult(res);
}
