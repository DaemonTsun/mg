
#include <assert.h>
#include "mg/impl/vk_image.hpp"

void mg::init(mg::vk_image *vimg, VkImage img, VkExtent3D extent, VkImageCreateFlags flags, VkImageType image_type, VkFormat format, u32 mipmap_levels, u32 array_layers, VkSampleCountFlagBits samples, VkImageTiling tiling, VkImageUsageFlags usage, VkSharingMode sharemode, VkImageLayout initial_layout)
{
    assert(vimg != nullptr);

    vimg->flags = flags;
    vimg->image_type = image_type;
    vimg->format = format;
    vimg->extent = extent;
    vimg->mipmap_levels = mipmap_levels;
    vimg->array_layers = array_layers;
    vimg->samples = samples;
    vimg->tiling = tiling;
    vimg->usage = usage;
    vimg->sharemode = sharemode;
    vimg->layout = initial_layout;
    
    vimg->image = img;
    vimg->memory = nullptr;
}

void mg::init(mg::vk_image *vimg, VkImage img, VkImageCreateInfo *info)
{
    assert(vimg != nullptr);

    vimg->flags = info->flags;
    vimg->image_type = info->imageType;
    vimg->format = info->format;
    vimg->extent = info->extent;
    vimg->mipmap_levels = info->mipLevels;
    vimg->array_layers = info->arrayLayers;
    vimg->samples = info->samples;
    vimg->tiling = info->tiling;
    vimg->usage = info->usage;
    vimg->sharemode = info->sharingMode;
    vimg->layout = info->initialLayout;
    
    vimg->image = img;
    vimg->memory = nullptr;
}

void mg::free(mg::vk_image *vimg)
{
    assert(vimg != nullptr);
}
