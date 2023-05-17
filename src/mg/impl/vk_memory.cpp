
#include <assert.h>

#include "shl/debug.hpp"
#include "shl/number_types.hpp"

#include "mg/vk_error.hpp"
#include "mg/impl/vk_buffer.hpp"
#include "mg/impl/vk_image.hpp"
#include "mg/impl/vk_memory.hpp"
    
struct insertion_t
{
    bool valid;
    u64 index;
    VkDeviceSize offset;
};

insertion_t find_free_space(mg::vk_memory *mem, VkDeviceSize size, VkDeviceSize alignment)
{
    insertion_t ret;
    ret.valid = true;
    ret.index = 0;
    ret.offset = 0;
    
    VkDeviceSize prev_end = 0;
    VkDeviceSize diff_size = 0;
    
    for_list(i, binding, &mem->bindings)
    {
        diff_size = binding->range.offset - prev_end;
        
        if (diff_size >= size)
            break;
        
        prev_end = mg::align_next(end(&binding->range), alignment);
    }
    
    if (i >= mem->bindings.size)
    {
        diff_size = mem->size - prev_end;
        
        if (diff_size < size)
            ret.valid = false;
    }
    
    ret.index = i;
    ret.offset = prev_end;
    
    return ret;
}

void update_largest_contiguous_free_space(mg::vk_memory *mem)
{
    mem->largest_contiguous_free_space.size = 0;
    VkDeviceSize prev_end = 0;
    VkDeviceSize diff_size = 0;
    
    for_list(binding, &mem->bindings)
    {
        diff_size = binding->range.offset - prev_end;
        
        if (diff_size > mem->largest_contiguous_free_space.size)
        {
            mem->largest_contiguous_free_space.offset = prev_end;
            mem->largest_contiguous_free_space.size = diff_size;
        }
            
        prev_end = end(&binding->range);
    }
    
    if (prev_end < mem->size)
    {
        diff_size = mem->size - prev_end;
        
        if (diff_size > mem->largest_contiguous_free_space.size)
        {
            mem->largest_contiguous_free_space.offset = prev_end;
            mem->largest_contiguous_free_space.size = diff_size;
        }
    }
}

void mg::init(mg::vk_memory *mem, VkDeviceMemory memory, VkDeviceSize size, VkMemoryPropertyFlags type, mg::memory_binding_type binding_type, u32 type_index)
{
    assert(mem != nullptr);

    mem->memory = memory;
    mem->size = size;
    mem->type = type;
    mem->type_index = type_index;
    mem->binding_type = binding_type;
    mem->largest_contiguous_free_space.offset = 0;
    mem->largest_contiguous_free_space.size = size;
    mem->total_free_space = size;
    ::init(&mem->bindings);
}

void mg::bind_buffer_to_memory(mg::vk_memory *mem, VkDevice dev, mg::vk_buffer *buf)
{
    assert(mem != nullptr);
    assert(buf != nullptr);
    assert(buf->memory == nullptr);
    assert(buf->size <= mem->total_free_space); // does not guarantee buffer fits
    assert(mem->binding_type == mg::memory_binding_type::Buffer);
    
    VkMemoryRequirements reqs;
    vkGetBufferMemoryRequirements(dev, buf->buffer, &reqs);
    
    assert(mg::has_space_for(mem, reqs.size, reqs.alignment));
    
    insertion_t i = ::find_free_space(mem, reqs.size, reqs.alignment);
    
    if (!i.valid)
        throw_error("no free space large enough in vk_memory %p to fit vk_buffer %p with size %u and alignment %u", mem, buf, reqs.size, reqs.alignment);
    
    trace("binding buffer %p to memory %p (index %u) at offset %u with size %u\n", buf, mem->memory, mem->type_index, i.offset, reqs.size);
    vkBindBufferMemory(dev, buf->buffer, mem->memory, i.offset);
    
    buf->memory = mem;
    buf->offset = i.offset;
    
    auto node = ::insert_elements(&mem->bindings, i.index, 1);

    vk_memory_binding *binding = &node->value;
    binding->range.offset = i.offset;
    binding->range.size = reqs.size;
    binding->data.buffer = buf;

    mem->total_free_space -= reqs.size;
    ::update_largest_contiguous_free_space(mem);
}

void mg::unbind_buffer_from_memory(mg::vk_memory *mem, mg::vk_buffer *buf)
{
    assert(mem != nullptr);
    assert(buf != nullptr);
    assert(buf->memory == mem);
    assert(mem->binding_type == memory_binding_type::Buffer);
    
    for_list(i, binding, &mem->bindings)
        if (binding->data.buffer->buffer == buf->buffer)
            break;
        
    if (i >= mem->bindings.size)
        return;
    
    trace("unbinding buffer %p from memory %p (index %u) at offset %u with size %u\n", buf, mem->memory, mem->type_index, it->range.offset, it->range.size);

    mem->total_free_space += binding->range.size;
    ::remove_elements(&mem->bindings, i, 1);
    ::update_largest_contiguous_free_space(mem);
    
    buf->memory = nullptr;
    buf->offset = 0;
}

void mg::bind_image_to_memory(mg::vk_memory *mem, VkDevice dev, mg::vk_image *img)
{
    assert(mem != nullptr);
    assert(img != nullptr);
    assert(img->memory == nullptr);
    assert(mem->binding_type == mg::memory_binding_type::Image);
    
    VkMemoryRequirements reqs;
    vkGetImageMemoryRequirements(dev, img->image, &reqs);
    
    assert(mg::has_space_for(mem, reqs.size, reqs.alignment));
    
    insertion_t i = ::find_free_space(mem, reqs.size, reqs.alignment);
    
    if (!i.valid)
        throw_error("no free space large enough in vk_memory %p to fit vk_image %p with size %u and alignment %u", mem, img, reqs.size, reqs.alignment);
    
    trace("binding image %p to memory %p (index %u) at offset %u with size %u\n", img, mem->memory, mem->type_index, i.offset, reqs.size);
    vkBindImageMemory(dev, img->image, mem->memory, i.offset);
    
    img->memory = mem;
    img->offset = i.offset;
    
    auto node = ::insert_elements(&mem->bindings, i.index, 1);

    vk_memory_binding *binding = &node->value;
    binding->range.offset = i.offset;
    binding->range.size = reqs.size;
    binding->data.image = img;

    mem->total_free_space -= reqs.size;
    ::update_largest_contiguous_free_space(mem);
}

void mg::unbind_image_from_memory(mg::vk_memory *mem, mg::vk_image *img)
{
    assert(mem != nullptr);
    assert(img != nullptr);
    assert(img->memory == mem);
    assert(mem->binding_type == memory_binding_type::Image);
    
    for_list(i, binding, &mem->bindings)
        if (binding->data.image->image == img->image)
            break;
        
    if (i >= mem->bindings.size)
        return;
    
    trace("unbinding image %p from memory %p (index %u) at offset %u with size %u\n", img, mem->memory, mem->type_index, it->range.offset, it->range.size);

    mem->total_free_space += binding->range.size;
    ::remove_elements(&mem->bindings, i, 1);
    ::update_largest_contiguous_free_space(mem);
    
    img->memory = nullptr;
    img->offset = 0;
}

bool mg::has_space_for(mg::vk_memory *mem, VkDeviceSize size, VkDeviceSize alignment)
{
    assert(mem != nullptr);
    return mg::has_space_for(&mem->largest_contiguous_free_space, size, alignment);
}

void mg::free(mg::vk_memory *mem)
{
    assert(mem != nullptr);

    ::free(&mem->bindings);
}
