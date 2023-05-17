
#include <assert.h>

#include "shl/compare.hpp"
#include "shl/debug.hpp"
#include "shl/number_types.hpp"

#include "mg/vk_error.hpp"
#include "mg/impl/context.hpp"
#include "mg/impl/memory_manager.hpp"

void mg::init(mg::memory_manager *mgr, mg::context *ctx)
{
    assert(mgr != nullptr);

    trace("setting up memory manager %p\n", mgr);

    mgr->context = ctx;
    mg::init(&mgr->allocator, ctx);
    ::init(&mgr->buffers);
    ::init(&mgr->images);

    mgr->min_allocation_size = mg::DEFAULT_MIN_ALLOC_SIZE;
}

void mg::free(mg::memory_manager *mgr)
{
    mg::destroy_all_buffers(mgr);
    mg::destroy_all_images(mgr);

    mg::free(&mgr->allocator);
    mgr->context = nullptr;
}

mg::vk_sub_buffer *mg::get_new_sub_buffer(mg::memory_manager *mgr, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharemode)
{
    mg::vk_buffer *buf = nullptr;

    for_list(tmp, &mgr->buffers)
        if ((tmp->usage & usage) == usage
          && tmp->sharemode == sharemode
          && mg::has_space_for(tmp, size))
        {
            buf = tmp;
            break;
        }

    if (buf == nullptr)
        buf = mg::create_buffer(mgr, AUTO_SIZE, usage, sharemode);

    return mg::create_sub_buffer(buf, size);
}

mg::vk_sub_buffer *mg::get_new_bound_sub_buffer(mg::memory_manager *mgr, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memflags, VkSharingMode sharemode)
{
    mg::vk_buffer *buf = nullptr;

    for_list(tmp, &mgr->buffers)
        if ((tmp->usage & usage) == usage
         && tmp->sharemode == sharemode
         && tmp->memory != nullptr
         && (tmp->memory->type & memflags) == memflags
         && mg::has_space_for(tmp, size))
        {
            buf = tmp;
            break;
        }

    if (buf == nullptr)
    {
        buf = mg::create_buffer(mgr, AUTO_SIZE, usage, sharemode);
        mg::auto_bind_buffer(mgr, buf, memflags);
    }

    return mg::create_sub_buffer(buf, size);
}

mg::vk_sub_buffer *mg::get_new_device_local_sub_buffer(mg::memory_manager *mgr, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharemode)
{
    return mg::get_new_bound_sub_buffer(mgr, size, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sharemode);
}

mg::vk_sub_buffer *mg::get_new_host_sub_buffer(mg::memory_manager *mgr, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharemode)
{
    return mg::get_new_bound_sub_buffer(mgr, size, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, sharemode);
}

mg::vk_sub_buffer *mg::get_new_host_coherent_sub_buffer(mg::memory_manager *mgr, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharemode)
{
    return mg::get_new_bound_sub_buffer(mgr, size, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sharemode);
}

mg::vk_buffer *mg::create_buffer(mg::memory_manager *mgr, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharemode)
{
    assert(mgr != nullptr);

    if (size == AUTO_SIZE)
        size = mgr->min_allocation_size;
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = sharemode;
    
    VkBuffer buf;
    VkResult res = vkCreateBuffer(mgr->context->device, &bufferInfo, nullptr, &buf);
    
    if (res != VK_SUCCESS)
        throw_vk_error(res, "%p could not create buffer", mgr);
    
    auto node = ::add_at_end(&mgr->buffers);
    mg::vk_buffer *ret = &node->value;
    mg::init(ret, buf, size, 0, usage, sharemode);

    return ret;
}

mg::vk_buffer *mg::create_uniform_buffer(mg::memory_manager *mgr, VkDeviceSize size, VkBufferUsageFlags additional_flags, VkSharingMode sharemode)
{
    return mg::create_buffer(mgr, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | additional_flags, sharemode);
}

mg::vk_buffer *mg::create_storage_buffer(mg::memory_manager *mgr, VkDeviceSize size, VkBufferUsageFlags additional_flags, VkSharingMode sharemode)
{
    return mg::create_buffer(mgr, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | additional_flags, sharemode);
}

mg::vk_buffer *mg::create_index_buffer(mg::memory_manager *mgr, VkDeviceSize size, VkBufferUsageFlags additional_flags, VkSharingMode sharemode)
{
    return mg::create_buffer(mgr, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | additional_flags, sharemode);
}

mg::vk_buffer *mg::create_vertex_buffer(mg::memory_manager *mgr, VkDeviceSize size, VkBufferUsageFlags additional_flags, VkSharingMode sharemode)
{
    return mg::create_buffer(mgr, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | additional_flags, sharemode);
}

void mg::auto_bind_buffer(mg::memory_manager *mgr, mg::vk_buffer *buf, VkMemoryPropertyFlags flags)
{
    assert(mgr != nullptr);
    assert(buf != nullptr);
    assert(buf->buffer != nullptr);
    assert(buf->memory == nullptr);
    
    VkMemoryRequirements reqs;
    vkGetBufferMemoryRequirements(mgr->context->device, buf->buffer, &reqs);
    
    u32 memtypeindex = mg::find_memory_type_index(&mgr->allocator, flags, reqs.memoryTypeBits);
    mg::vk_memory *memory = nullptr;
    
    for_list(_mem, &mgr->allocator.allocated_memory)
    {
        if (_mem->type_index != memtypeindex
         || _mem->binding_type != mg::memory_binding_type::Buffer
         || !mg::has_space_for(_mem, reqs.size, reqs.alignment))
            continue;
        
        trace("suitable memory found: %u bytes, index %u\n", mem->size, mem->type_index);
        memory = _mem;
    }
        
    if (memory == nullptr)
    {
        const VkDeviceSize newsz = Max(reqs.size, mgr->min_allocation_size);
        trace("no suitable memory found, allocating mem: %u bytes, index %u\n", newsz, memtypeindex);
        memory = mg::allocate_memory_by_memory_type_index(&mgr->allocator, newsz, mg::memory_binding_type::Buffer, memtypeindex);
    }
    
    mg::bind_buffer_to_memory(memory, mgr->context->device, buf);
}

void mg::auto_bind_device_local_buffer(mg::memory_manager *mgr, mg::vk_buffer *buf)
{
    mg::auto_bind_buffer(mgr, buf, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void mg::auto_bind_host_buffer(mg::memory_manager *mgr, mg::vk_buffer *buf)
{
    mg::auto_bind_buffer(mgr, buf, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
}

void mg::auto_bind_host_coherent_buffer(mg::memory_manager *mgr, mg::vk_buffer *buf)
{
    mg::auto_bind_buffer(mgr, buf, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void destroy_buffer(mg::memory_manager *mgr, mg::vk_buffer *buf)
{
    if (buf->memory != nullptr)
        mg::unbind_buffer_from_memory(buf->memory, buf);

    if (buf->buffer != nullptr)
        vkDestroyBuffer(mgr->context->device, buf->buffer, nullptr);
    
    mg::free(buf);
}

void mg::destroy_buffer(mg::memory_manager *mgr, mg::vk_buffer *buf)
{
    assert(mgr != nullptr);
    assert(buf != nullptr);
    
    for_list(i, tmp, &mgr->buffers)
        if (tmp->buffer == buf->buffer)
            break;

    if (i < mgr->buffers.size)
    {
        ::destroy_buffer(mgr, tmp);
        ::remove_elements(&mgr->buffers, i, 1);
    }
}

void mg::destroy_all_buffers(mg::memory_manager *mgr)
{
    assert(mgr != nullptr);

    for_list(buf, &mgr->buffers)
        ::destroy_buffer(mgr, buf);
    
    ::clear(&mgr->buffers);
}

mg::vk_image *mg::create_image(mg::memory_manager *mgr, VkImageCreateInfo *info)
{
    assert(mgr != nullptr);
    assert(info != nullptr);

    VkImage img;
    VkResult res = vkCreateImage(mgr->context->device, info, nullptr, &img);

    if (res != VK_SUCCESS)
        throw_vk_error(res, "%p could not create image", mgr);

    auto node = ::add_at_end(&mgr->images);
    mg::vk_image *ret = &node->value;
    mg::init(ret, img, info);

    return ret;
}

mg::vk_image *mg::create_image(mg::memory_manager *mgr, VkExtent3D extent, VkImageCreateFlags flags, VkImageType image_type, VkFormat format, u32 mipmap_levels, u32 array_layers, VkSampleCountFlagBits samples, VkImageTiling tiling, VkImageUsageFlags usage, VkSharingMode sharemode, VkImageLayout initial_layout)
{
    assert(mgr != nullptr);

    VkImageCreateInfo info;
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;
    info.imageType = image_type;
    info.format = format;
    info.extent = extent;
    info.mipLevels = mipmap_levels;
    info.arrayLayers = array_layers;
    info.samples = samples;
    info.tiling = tiling;

    info.usage = usage;
    info.sharingMode = sharemode;
    info.queueFamilyIndexCount = 0;
    info.pQueueFamilyIndices = nullptr;

    info.initialLayout = initial_layout;

    return mg::create_image(mgr, &info);
}

void mg::auto_bind_image(mg::memory_manager *mgr, mg::vk_image *img, VkMemoryPropertyFlags flags)
{
    assert(mgr != nullptr);
    assert(img != nullptr);
    assert(img->image != nullptr);
    assert(img->memory == nullptr);
    
    VkMemoryRequirements reqs;
    vkGetImageMemoryRequirements(mgr->context->device, img->image, &reqs);
    
    u32 memtypeindex = mg::find_memory_type_index(&mgr->allocator, flags, reqs.memoryTypeBits);
    mg::vk_memory *memory = nullptr;
    
    for_list(_mem, &mgr->allocator.allocated_memory)
    {
        if (_mem->type_index != memtypeindex
         || _mem->binding_type != mg::memory_binding_type::Image
         || !mg::has_space_for(_mem, reqs.size, reqs.alignment))
            continue;
        
        trace("suitable memory found: %u bytes, index %u\n", mem->size, mem->type_index);
        memory = _mem;
    }
        
    if (memory == nullptr)
    {
        const VkDeviceSize newsz = Max(reqs.size, mgr->min_allocation_size);
        trace("no suitable memory found, allocating mem: %u bytes, index %u\n", newsz, memtypeindex);
        memory = mg::allocate_memory_by_memory_type_index(&mgr->allocator, newsz, mg::memory_binding_type::Image, memtypeindex);
    }
    
    mg::bind_image_to_memory(memory, mgr->context->device, img);
}

void mg::auto_bind_device_local_image(mg::memory_manager *mgr, mg::vk_image *img)
{
    mg::auto_bind_image(mgr, img, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void mg::auto_bind_host_image(mg::memory_manager *mgr, mg::vk_image *img)
{
    mg::auto_bind_image(mgr, img, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
}

void mg::auto_bind_host_coherent_image(mg::memory_manager *mgr, mg::vk_image *img)
{
    mg::auto_bind_image(mgr, img, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void destroy_image(mg::memory_manager *mgr, mg::vk_image *img)
{
    if (img->memory != nullptr)
        mg::unbind_image_from_memory(img->memory, img);

    if (img->image != nullptr)
        vkDestroyImage(mgr->context->device, img->image, nullptr);
    
    mg::free(img);
}

void mg::destroy_image(mg::memory_manager *mgr, mg::vk_image *img)
{
    assert(mgr != nullptr);
    assert(img != nullptr);
    
    for_list(i, tmp, &mgr->images)
        if (tmp->image == img->image)
            break;

    if (i < mgr->images.size)
    {
        ::destroy_image(mgr, tmp);
        ::remove_elements(&mgr->images, i, 1);
    }
}

void mg::destroy_all_images(mg::memory_manager *mgr)
{
    assert(mgr != nullptr);

    for_list(img, &mgr->images)
        ::destroy_image(mgr, img);
    
    ::clear(&mgr->images);
}
