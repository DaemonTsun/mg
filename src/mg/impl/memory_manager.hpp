
#pragma once

#include <vulkan/vulkan_core.h>

#include "shl/linked_list.hpp"

#include "mg/impl/memory_allocator.hpp"
#include "mg/impl/vk_buffer.hpp"
#include "mg/impl/vk_image.hpp"

namespace mg
{
struct context;
constexpr const VkDeviceSize DEFAULT_MIN_ALLOC_SIZE = 16777216ull;
constexpr const VkDeviceSize AUTO_SIZE = -2ull;

typedef linked_list<mg::vk_buffer> buffer_list;
typedef linked_list<mg::vk_image>  image_list;

struct memory_manager
{
    // the size of new automatic memory allocations if requested size
    // is smaller than or equal to this size.
    VkDeviceSize min_allocation_size;    

    mg::context *context;
    mg::memory_allocator allocator;
    mg::buffer_list buffers;
    mg::image_list images;
};
    
void init(mg::memory_manager *mgr, context *ctx);
void free(mg::memory_manager *mgr);

// DOES allocate memory if none is available
// DOES create a buffer it none is available
mg::vk_sub_buffer *get_new_sub_buffer(mg::memory_manager *mgr, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharemode = VK_SHARING_MODE_EXCLUSIVE);
mg::vk_sub_buffer *get_new_bound_sub_buffer(mg::memory_manager *mgr, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memflags, VkSharingMode sharemode = VK_SHARING_MODE_EXCLUSIVE);
mg::vk_sub_buffer *get_new_device_local_sub_buffer(mg::memory_manager *mgr, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharemode = VK_SHARING_MODE_EXCLUSIVE);
mg::vk_sub_buffer *get_new_host_coherent_sub_buffer(mg::memory_manager *mgr, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharemode = VK_SHARING_MODE_EXCLUSIVE);
mg::vk_sub_buffer *get_new_host_sub_buffer(mg::memory_manager *mgr, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharemode = VK_SHARING_MODE_EXCLUSIVE);

// does not allocate memory, only creates a buffer
mg::vk_buffer *create_buffer(mg::memory_manager *mgr, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharemode = VK_SHARING_MODE_EXCLUSIVE);
mg::vk_buffer *create_uniform_buffer(mg::memory_manager *mgr, VkDeviceSize size = AUTO_SIZE, VkBufferUsageFlags additional_flags = 0, VkSharingMode sharemode = VK_SHARING_MODE_EXCLUSIVE);
mg::vk_buffer *create_storage_buffer(mg::memory_manager *mgr, VkDeviceSize size = AUTO_SIZE, VkBufferUsageFlags additional_flags = 0, VkSharingMode sharemode = VK_SHARING_MODE_EXCLUSIVE);
mg::vk_buffer *create_index_buffer  (mg::memory_manager *mgr, VkDeviceSize size = AUTO_SIZE, VkBufferUsageFlags additional_flags = 0, VkSharingMode sharemode = VK_SHARING_MODE_EXCLUSIVE);
mg::vk_buffer *create_vertex_buffer (mg::memory_manager *mgr, VkDeviceSize size = AUTO_SIZE, VkBufferUsageFlags additional_flags = 0, VkSharingMode sharemode = VK_SHARING_MODE_EXCLUSIVE);

// searches for available, unbound memory matching the requirements of
// the buffer and binds the memory to the buffer.
// allocates memory if no already allocated memory is available.
// throws if something goes wrong.
void auto_bind_buffer(mg::memory_manager *mgr, mg::vk_buffer *buf, VkMemoryPropertyFlags flags);
void auto_bind_device_local_buffer(mg::memory_manager *mgr, mg::vk_buffer *buf);
void auto_bind_host_buffer(mg::memory_manager *mgr, mg::vk_buffer *buf);
void auto_bind_host_coherent_buffer(mg::memory_manager *mgr, mg::vk_buffer *buf);

void destroy_buffer(mg::memory_manager *mgr, mg::vk_buffer *buf);
void destroy_all_buffers(mg::memory_manager *mgr);

// images
mg::vk_image *create_image(mg::memory_manager *mgr, VkImageCreateInfo *info);
mg::vk_image *create_image(mg::memory_manager *mgr, VkExtent3D extent, VkImageCreateFlags flags = 0, VkImageType image_type = VK_IMAGE_TYPE_2D, VkFormat format = VK_FORMAT_R8G8B8A8_UINT, u32 mipmap_levels = 1, u32 array_layers = 1, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, VkImageUsageFlags usage = 0, VkSharingMode sharemode = VK_SHARING_MODE_EXCLUSIVE, VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED);

void auto_bind_image(mg::memory_manager *mgr, mg::vk_image *img, VkMemoryPropertyFlags flags);
void auto_bind_device_local_image(mg::memory_manager *mgr, mg::vk_image *img);
void auto_bind_host_image(mg::memory_manager *mgr, mg::vk_image *img);
void auto_bind_host_coherent_image(mg::memory_manager *mgr, mg::vk_image *img);

void destroy_image(mg::memory_manager *mgr, mg::vk_image *img);
void destroy_all_images(mg::memory_manager *mgr);
}
