
#pragma once

#include <assert.h>
#include <vulkan/vulkan_core.h>

#include "shl/array.hpp"
#include "shl/string.hpp"
#include "shl/number_types.hpp"

#include "mg/impl/vk_buffer.hpp"
#include "mg/impl/descriptor_pool_manager.hpp"
#include "mg/impl/memory_manager.hpp"
#include "mg/window.hpp"
#include "mg/context.hpp"

#define MAX_FRAMES_IN_FLIGHT 2

namespace mg
{
struct vk_config
{
    bool debug;
    bool enable_layers;
    
    array<const char*> device_extension_names;
    
    u32 graphics_queue_count;
    array<float> graphics_queue_priorities;
    
    struct _surface
    {
        u32 min_swap_image_count;
        VkExtent2D extent;
        VkSurfaceFormatKHR format;
    } surface;

    VkViewport viewport;
    VkRect2D scissor;
    
    struct _swap
    {
        VkPresentModeKHR present_mode;
        VkPresentModeKHR fallback_present_mode;
        VkSurfaceTransformFlagBitsKHR transform;
    } swap;
};

// sets default values for a config
void init(mg::vk_config *conf);
void free(mg::vk_config *conf);

struct swap_buffer_data
{
    mg::vk_sub_buffer *source;
    mg::vk_sub_buffer *destination;
    VkDeviceSize size;
};

struct image_swap_buffer_data
{
    mg::vk_sub_buffer *source;
    mg::vk_image *destination;

    u32                source_width;
    u32                source_height;
    VkOffset3D         destination_offset;
    VkImageAspectFlags aspect;
    u32                mipmap_level;
    u32                layer_start;
    u32                layer_count;
};

typedef array<mg::swap_buffer_data> swap_buffer_list;
typedef array<mg::image_swap_buffer_data> image_swap_buffer_list;

struct frame_data
{
	VkFence render_fence;
    VkSemaphore present_semaphore;
    VkSemaphore render_semaphore;

	VkCommandPool command_pool;
    array<VkCommandBuffer> command_buffers;

    mg::descriptor_pool_manager descriptor_pool_manager;
};

struct context
{
    struct _changed
    {
        bool extent;
    } changed;

    vk_config config;

    mg::window *window;
    VkInstance instance;

    mg::swap_buffer_list swap_buffers;
    mg::image_swap_buffer_list image_swap_buffers;

    mg::memory_manager memory_manager;
    mg::descriptor_pool_manager descriptor_pool_manager;

    VkPhysicalDevice physical_device;
    
    u32 graphics_queue_index;
    u32 graphics_queue_max_count;
    VkQueue graphics_queue;
    u32 present_queue_index;
    u32 present_queue_max_count;
    VkQueue present_queue;
    
    VkDevice device;
    VkSurfaceKHR target_surface;
    VkSwapchainKHR swapchain;

    array<VkImage> swapchain_images;
    array<VkImageView> swapchain_image_views;
    array<VkFence> swapchain_image_fences;

    VkRenderPass render_pass;
    u32 current_image_index; // used in between start_rendering and end_rendering

    array<VkFramebuffer> framebuffers;
    mg::frame_data frames[MAX_FRAMES_IN_FLIGHT];

    // between 0 and MAX_FRAMES_IN_FLIGHT - 1;
    u32 current_frame;

    mg::time_data time_data;
};

void init(mg::context *ctx);

// layer names must be freed
void get_vulkan_layer_names(array<const char*> *out_layers, const array<const_string> *layer_exceptions);
VkPresentModeKHR get_supported_present_mode(const context *ctx);

void submit_immediate_vulkan_command_to_current_frame(mg::context *ctx, void (*cmd)(VkCommandBuffer, void*), void *userdata);

// only possible on host coherent buffers
void write_buffer(mg::context *ctx, mg::vk_sub_buffer *dest, const void *data, u64 size);
// possible on any writable buffers
void queue_buffer_upload(mg::context *ctx, mg::vk_sub_buffer *dest, const void *data, u64 size);
void queue_image_upload(mg::context *ctx, mg::vk_image *dest, const void *data, u64 data_size, u32 width, u32 height, VkOffset3D offset = {0, 0, 0}, VkImageAspectFlags aspects = VK_IMAGE_ASPECT_COLOR_BIT, u32 mipmap_level = 0, u32 layer_start = 0, u32 layer_count = 1);
void upload_queued_buffers(mg::context *ctx); // is called automatically in update(ctx)
void clear_queued_buffers(mg::context *ctx);

void setup_instance(mg::context *ctx, const char** extensions, u32 extension_count);
void set_physical_device(mg::context *ctx);
void set_min_swap_image_count(mg::context *ctx, u32 default_min = UINT32_MAX);
void set_present_queue(mg::context *ctx);
void create_logical_device(mg::context *ctx, const array<const_string> *layer_exceptions = nullptr);
void create_swapchain(mg::context *ctx);
void set_swapchain_images(mg::context *ctx);
void create_swapchain_image_views(mg::context *ctx);
void create_render_pass(mg::context *ctx);
void create_framebuffers(mg::context *ctx);
void create_frame_data(mg::context *ctx);

void destroy_frame_data(mg::context *ctx);
void destroy_framebuffers(mg::context *ctx);
void destroy_render_pass(mg::context *ctx);
void destroy_depth_buffers(mg::context *ctx);
void destroy_swapchain_image_views(mg::context *ctx);
void destroy_swapchain(mg::context *ctx);
void destroy_target_surface(mg::context *ctx);
void destroy_descriptor_pool_manager(mg::context *ctx);
void destroy_memory_manager(mg::context *ctx);
void destroy_logical_device(mg::context *ctx);
void destroy_vulkan_instance(mg::context *ctx);

void free(mg::context *ctx);
}
