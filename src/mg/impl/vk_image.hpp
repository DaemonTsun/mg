
#pragma once

#include <vulkan/vulkan_core.h>

#include "shl/number_types.hpp"
#include "mg/impl/vk_memory.hpp"

namespace mg
{
struct vk_image
{
    VkImage image;
    vk_memory *memory;
    
    VkDeviceSize          offset;
    
    VkImageCreateFlags    flags;
    VkImageType           image_type;
    VkFormat              format;
    VkExtent3D            extent;
    u32                   mipmap_levels;
    u32                   array_layers;
    VkSampleCountFlagBits samples;
    VkImageTiling         tiling;
    VkImageUsageFlags     usage;
    VkSharingMode         sharemode;
    VkImageLayout         layout;
};

void init(mg::vk_image *vimg, VkImage img, VkExtent3D extent, VkImageCreateFlags flags = 0, VkImageType image_type = VK_IMAGE_TYPE_2D, VkFormat format = VK_FORMAT_R8G8B8A8_UINT, u32 mipmap_levels = 1, u32 array_layers = 1, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, VkImageUsageFlags usage = 0, VkSharingMode sharemode = VK_SHARING_MODE_EXCLUSIVE, VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED);
void init(mg::vk_image *vimg, VkImage img, VkImageCreateInfo *info);
void free(mg::vk_image *vimg);
}
