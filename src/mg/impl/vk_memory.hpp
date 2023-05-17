#pragma once

#include <vulkan/vulkan_core.h>

#include "shl/linked_list.hpp"
#include "shl/number_types.hpp"

#include "mg/number_range.hpp"

namespace mg
{
struct vk_buffer;
struct vk_image;

enum class memory_binding_type : u8
{
    Buffer,
    Image
};

typedef number_range<VkDeviceSize> bind_range;

struct vk_memory_binding
{
    mg::bind_range range;

    union _data
    {
        mg::vk_buffer *buffer;
        mg::vk_image *image;
    } data;
};

typedef linked_list<mg::vk_memory_binding> binding_list;

struct vk_memory
{
    VkDeviceMemory memory;
    VkDeviceSize size;
    VkMemoryPropertyFlags type;
    u32 type_index;
    mg::memory_binding_type binding_type;
    
    mg::bind_range largest_contiguous_free_space;
    VkDeviceSize total_free_space;

    // internal things
    mg::binding_list bindings;
};

void init(mg::vk_memory *mem, VkDeviceMemory memory = nullptr, VkDeviceSize size = 0, VkMemoryPropertyFlags type = 0, mg::memory_binding_type binding_type = mg::memory_binding_type::Buffer, u32 type_index = 0);

void bind_buffer_to_memory(mg::vk_memory *mem, VkDevice dev, mg::vk_buffer *buf);
void unbind_buffer_from_memory(mg::vk_memory *mem, mg::vk_buffer *buf);

void bind_image_to_memory(mg::vk_memory *mem, VkDevice dev, mg::vk_image *buf);
void unbind_image_from_memory(mg::vk_memory *mem, mg::vk_image *buf);

bool has_space_for(mg::vk_memory *mem, VkDeviceSize size, VkDeviceSize alignment);

void free(mg::vk_memory *mem);
}
