
#include <assert.h>

#include "shl/debug.hpp"
#include "shl/number_types.hpp"

#include "mg/vk_error.hpp"
#include "mg/impl/vk_buffer.hpp"

struct insertion_t
{
    bool valid;
    u64 index;
    VkDeviceSize offset;
};

insertion_t find_free_space(mg::vk_buffer *buf, VkDeviceSize size, VkDeviceSize alignment)
{
    assert(buf != nullptr);
    insertion_t ret;
    
    VkDeviceSize prev_end = 0;
    VkDeviceSize diff_size = 0;
    
    for_list(i, sb, &buf->sub_buffers)
    {
        diff_size = sb->range.offset - prev_end;
        
        if (diff_size >= size)
            break;
        
        prev_end = mg::align_next(end(&sb->range), alignment);
    }
    
    ret.valid = true;
    
    if (i >= buf->sub_buffers.size)
    {
        diff_size = buf->size - prev_end;
        
        if (diff_size < size)
            ret.valid = false;
    }
    
    ret.index = i;
    ret.offset = prev_end;
    
    return ret;
}

void update_largest_contiguous_free_space(mg::vk_buffer *buf)
{
    // TODO: fix calculation
    assert(buf != nullptr);
    buf->largest_contiguous_free_space.size = 0;
    VkDeviceSize prev_end = 0;
    VkDeviceSize diff_size = 0;
    
    for_list(sb, &buf->sub_buffers)
    {
        diff_size = sb->range.offset - prev_end;
        
        if (diff_size > buf->largest_contiguous_free_space.size)
        {
            buf->largest_contiguous_free_space.offset = prev_end;
            buf->largest_contiguous_free_space.size = diff_size;
        }
            
        prev_end = end(&sb->range);
    }
    
    if (prev_end < buf->size)
    {
        diff_size = buf->size - prev_end;
        
        if (diff_size > buf->largest_contiguous_free_space.size)
        {
            buf->largest_contiguous_free_space.offset = prev_end;
            buf->largest_contiguous_free_space.size = diff_size;
        }
    }
}

void mg::init(mg::vk_buffer *buf, VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset, VkBufferUsageFlags usage, VkSharingMode sharemode)
{
    assert(buf != nullptr);

    buf->buffer = buffer;
    buf->memory = nullptr;
    buf->offset = offset;
    buf->size = size;
    buf->usage = usage;
    buf->sharemode = sharemode;
    buf->largest_contiguous_free_space.offset = 0;
    buf->largest_contiguous_free_space.size = size;
    buf->total_free_space = size;

    ::init(&buf->sub_buffers);
}

void mg::free(mg::vk_buffer *buf)
{
    assert(buf != nullptr);

    ::free(&buf->sub_buffers);
}

mg::vk_sub_buffer *mg::create_sub_buffer(mg::vk_buffer *buf, VkDeviceSize size, VkDeviceSize alignment)
{
    assert(buf != nullptr);
    assert(mg::has_space_for(buf, size, alignment));
    
    insertion_t i = ::find_free_space(buf, size, alignment);
    
    if (!i.valid)
        throw_error("no free space large enough in vk_buffer %p to fit %u bytes with alignment %u", buf, size, alignment);
    
    trace("creating new sub-buffer for buffer %p at offset %u with size %u", buf->buffer, i.offset, size);
    
    auto node = ::insert_elements(&buf->sub_buffers, i.index, 1);
    vk_sub_buffer *sb = &node->value;
    sb->buffer = buf;
    sb->range.offset = i.offset;
    sb->range.size = size;
    
    buf->total_free_space -= size;
    ::update_largest_contiguous_free_space(buf);
    return sb;
}

void mg::destroy_sub_buffer(mg::vk_sub_buffer *sb)
{
    assert(sb != nullptr);
    mg::destroy_sub_buffer(sb->buffer, sb->range.offset);
}

void mg::destroy_sub_buffer(mg::vk_buffer *buf, VkDeviceSize offset)
{
    assert(buf != nullptr);

    u64 index = -1;

    for_list(i, tmp, &buf->sub_buffers)
        if (tmp->range.offset == offset)
        {
            index = i;
            break;
        }
        else if (tmp->range.offset > offset)
            break;
        
    if (index >= buf->sub_buffers.size)
        return;
    
    buf->total_free_space += tmp->range.size;
    ::remove_elements(&buf->sub_buffers, index, 1);
    ::update_largest_contiguous_free_space(buf);
}

void mg::destroy_all_sub_buffers(mg::vk_buffer *buf)
{
    assert(buf != nullptr);

    buf->largest_contiguous_free_space.offset = 0;
    buf->largest_contiguous_free_space.size = buf->size;
    buf->total_free_space = buf->size;
    ::clear(&buf->sub_buffers);
}

bool mg::has_space_for(mg::vk_buffer *buf, VkDeviceSize size, VkDeviceSize alignment)
{
    return mg::has_space_for(&buf->largest_contiguous_free_space, size, alignment);
}

VkDeviceSize mg::total_memory_offset(const mg::vk_sub_buffer *sub)
{
    return sub->range.offset + sub->buffer->offset;
}
