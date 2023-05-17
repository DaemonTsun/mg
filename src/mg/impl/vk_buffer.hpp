
#pragma once

#include "shl/linked_list.hpp"

#include "mg/number_range.hpp"
#include "mg/impl/vk_memory.hpp"

namespace mg
{
class vk_buffer;

typedef number_range<VkDeviceSize> sub_buffer_range;

struct vk_sub_buffer
{
    mg::vk_buffer *buffer; // parent
    mg::sub_buffer_range range;
};

typedef linked_list<vk_sub_buffer> sub_buffer_list;

struct vk_buffer
{
    VkBuffer buffer;
    mg::vk_memory *memory;
    
    VkDeviceSize offset; // pretty much always 0
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkSharingMode sharemode;
    
    mg::sub_buffer_range largest_contiguous_free_space;
    VkDeviceSize total_free_space;
    mg::sub_buffer_list sub_buffers;
};

void init(mg::vk_buffer *buf, VkBuffer buffer = nullptr, VkDeviceSize size = 0, VkDeviceSize offset = 0, VkBufferUsageFlags usage = 0, VkSharingMode share = VK_SHARING_MODE_EXCLUSIVE);
void free(mg::vk_buffer *buf);

// theres no GPU allocation going on here
mg::vk_sub_buffer *create_sub_buffer(mg::vk_buffer *buf, VkDeviceSize sz, VkDeviceSize alignment = 1);

void destroy_sub_buffer(vk_sub_buffer *sb);
void destroy_sub_buffer(mg::vk_buffer *buf, VkDeviceSize offset); // offset must match exactly
void destroy_all_sub_buffers(mg::vk_buffer *buf);

bool has_space_for(mg::vk_buffer *buf, VkDeviceSize size, VkDeviceSize alignment = 1);
    
// gets the total offset inside bound memory
// basically returns sub->offset + sub->buffer->offset
VkDeviceSize total_memory_offset(const vk_sub_buffer *sub);
}
