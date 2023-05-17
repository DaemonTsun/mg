
#include <string.h>

#include "shl/debug.hpp"
#include "shl/defer.hpp"
#include "shl/string.hpp"
#include "shl/array.hpp"
#include "shl/memory.hpp"

#include "mg/vk_error.hpp"
#include "mg/impl/context.hpp"

void mg::init(mg::vk_config *conf)
{
    assert(conf != nullptr);
    
    conf->debug = true,
    conf->enable_layers = true,
    
    ::init(&conf->device_extension_names);
#ifndef NDEBUG
    ::add_at_end(&conf->device_extension_names, "VK_KHR_shader_non_semantic_info");
#endif

    ::add_at_end(&conf->device_extension_names, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    
    conf->graphics_queue_count = 1;

    ::init(&conf->graphics_queue_priorities, 1);
    conf->graphics_queue_priorities[0] = 1.0f;
    
    conf->surface = mg::vk_config::_surface{
        .min_swap_image_count = 1,
        
        .extent {
            .width = 640,
            .height = 480
        },
        
        .format {
            .format = VK_FORMAT_B8G8R8A8_SRGB,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        }
    };

    conf->swap = mg::vk_config::_swap {
        .present_mode = VK_PRESENT_MODE_MAILBOX_KHR,
        .fallback_present_mode = VK_PRESENT_MODE_FIFO_KHR,
        .transform = VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR
    };

    conf->viewport.x = 0;
    conf->viewport.y = 0;
    conf->viewport.minDepth = 0.0f;
    conf->viewport.maxDepth = 1.0f;
    conf->viewport.width = 640;
    conf->viewport.height = 480;

    conf->scissor.offset = {0, 0};
    conf->scissor.extent = {640, 480};
}

void mg::free(mg::vk_config *conf)
{
    assert(conf != nullptr);

    free(&conf->device_extension_names);
    free(&conf->graphics_queue_priorities);
}

void mg::init(mg::context *ctx)
{
    assert(ctx != nullptr);

    ctx->changed.extent = false;

    mg::init(&ctx->config);

    ctx->window = nullptr;
    ctx->instance = nullptr;

    ::init(&ctx->swap_buffers);
    ::init(&ctx->image_swap_buffers);

    ctx->physical_device = nullptr;
    
    ctx->graphics_queue_index = UINT32_MAX;
    ctx->graphics_queue_max_count = 0;
    ctx->graphics_queue = nullptr;
    ctx->present_queue_index = UINT32_MAX;
    ctx->present_queue_max_count = 0;
    ctx->present_queue = nullptr;
    
    ctx->device = nullptr;
    ctx->target_surface = nullptr;
    ctx->swapchain = nullptr;

    ::init(&ctx->swapchain_images);
    ::init(&ctx->swapchain_image_views);
    ::init(&ctx->swapchain_image_fences);

    ctx->render_pass = nullptr;

    ::init(&ctx->framebuffers);

    ctx->current_frame = 0;

    ctx->time_data.elapsed_time = 0.f;
    ctx->time_data.total_frame_count = 0;

    for (u64 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        mg::frame_data *frame = ctx->frames +i;

        ::init(&frame->command_buffers);
    }
}

VkInstance create_vk_instance(const char **extensions, u32 extension_count, const char **layers, u32 layer_count)
{
    VkApplicationInfo app_info;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = "";
    app_info.applicationVersion = 0;
    app_info.pEngineName = "";
    app_info.engineVersion = 0;
    app_info.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo inst_info;
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = nullptr;
    inst_info.flags = 0;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledLayerCount = layer_count;
    inst_info.ppEnabledLayerNames = layers;
    inst_info.enabledExtensionCount = extension_count;
    inst_info.ppEnabledExtensionNames = extensions;

    VkInstance ret;
    VkResult res = vkCreateInstance(&inst_info, NULL, &ret);
    
    switch (res)
    {
    case VK_SUCCESS:
        break;
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        throw_vk_error(res, "unable to create vulkan instance, cannot find a compatible Vulkan ICD");
        
    default:
        throw_vk_error(res, "unable to create Vulkan instance");
    }
    
    return ret;
}

void mg::get_vulkan_layer_names(array<const char*> *out_layers, const array<const_string> *layer_exceptions)
{
    // Figure out the amount of available layers
    // Layers are used for debugging / validation etc / profiling..
    u32 layer_count = 0;
    VkResult res = vkEnumerateInstanceLayerProperties(&layer_count, NULL);

    if (res != VK_SUCCESS)
        throw_vk_error(res, "unable to query vulkan instance layer property count");

    array<VkLayerProperties> layer_names;
    ::init(&layer_names, layer_count);
    defer { ::free(&layer_names); };

    res = vkEnumerateInstanceLayerProperties(&layer_count, layer_names.data);

    if (res != VK_SUCCESS)
        throw_vk_error(res, "unable to retrieve vulkan instance layer names");
    
    ::reserve(out_layers, layer_count);
    
    for_array(prop, &layer_names)
    {
        if (layer_exceptions != nullptr)
        {
            bool ignore = false;
            
            for_array(exc, layer_exceptions)
                if (compare_strings(*exc, prop->layerName) == 0)
                {
                    ignore = true;
                    break;
                }

            if (ignore)
                continue;
        }

        const char *to_add = strndup(prop->layerName, string_length(prop->layerName));
        ::add_at_end(out_layers, to_add);
    }
}

VkPresentModeKHR mg::get_supported_present_mode(const mg::context *ctx)
{
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_MAX_ENUM_KHR;
    
    array<VkPresentModeKHR> presentModes;
    ::init(&presentModes);
    defer { ::free(&presentModes); };
    
    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physical_device, ctx->target_surface, &presentModeCount, nullptr);

    if (presentModeCount > 0)
    {
        ::resize(&presentModes, presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physical_device, ctx->target_surface, &presentModeCount, presentModes.data);
    }
    
    bool supported = false;
    bool fallback_supported = false;
    
    for_array(availablePresentMode, &presentModes)
    {
        if (*availablePresentMode == ctx->config.swap.present_mode)
        {
            supported = true;
            present_mode = ctx->config.swap.present_mode;
            break;
        }
        
        if (*availablePresentMode == ctx->config.swap.fallback_present_mode)
            fallback_supported = true;
    }
    
    if (!supported)
    {
        if (fallback_supported)
        {
            trace("present mode %u not supported, using fallback mode %u\n", ctx->config.swap.present_mode, ctx->config.swap.fallback_present_mode);
            
            present_mode = ctx->config.swap.fallback_present_mode;
        }
    }
    
    return present_mode;
}

void mg::submit_immediate_vulkan_command_to_current_frame(mg::context *ctx, void (*cmd)(VkCommandBuffer, void*), void *userdata)
{
    assert(ctx != nullptr);
    assert(cmd != nullptr);
    
    mg::frame_data *frame = ctx->frames + ctx->current_frame;
    VkCommandBuffer commandBuffer = frame->command_buffers[0];

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    cmd(commandBuffer, userdata);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.pWaitSemaphores = &frame->present_semaphore;

    vkResetFences(ctx->device, 1, &frame->render_fence);
    vkQueueSubmit(ctx->graphics_queue, 1, &submitInfo, frame->render_fence);
    vkQueueWaitIdle(ctx->graphics_queue);
}

void mg::write_buffer(mg::context *ctx, mg::vk_sub_buffer *dest, const void *data, u64 size)
{
    assert(ctx != nullptr);
    assert(dest != nullptr);
    assert(data != nullptr);
    assert(size > 0);

    VkDeviceSize offset = mg::total_memory_offset(dest);
    
    void* pdata;
    vkMapMemory(ctx->device, dest->buffer->memory->memory, offset, size, 0, &pdata);
    memcpy(pdata, data, size);
    vkUnmapMemory(ctx->device, dest->buffer->memory->memory);
}

void mg::queue_buffer_upload(mg::context *ctx, mg::vk_sub_buffer *dest, const void *data, u64 size)
{
    assert(ctx != nullptr);
    assert(dest != nullptr);
    assert(data != nullptr);
    assert(size > 0);

    mg::vk_sub_buffer *sbuf = mg::get_new_host_coherent_sub_buffer(&ctx->memory_manager, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    mg::write_buffer(ctx, sbuf, data, size);

    mg::swap_buffer_data *bdata = ::add_at_end(&ctx->swap_buffers);
    bdata->source = sbuf;
    bdata->destination = dest;
    bdata->size = size;
}

void mg::queue_image_upload(mg::context *ctx, mg::vk_image *dest, const void *data, u64 data_size, u32 width, u32 height, VkOffset3D offset, VkImageAspectFlags aspects, u32 mipmap_level, u32 layer_start, u32 layer_count)
{
    assert(ctx != nullptr);
    assert(dest != nullptr);
    assert(data != nullptr);
    assert(data_size > 0);
    assert(width > 0);
    assert(height > 0);

    mg::vk_sub_buffer *sbuf = mg::get_new_host_coherent_sub_buffer(&ctx->memory_manager, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    mg::write_buffer(ctx, sbuf, data, data_size);

    mg::image_swap_buffer_data *idata = ::add_at_end(&ctx->image_swap_buffers);
    idata->source = sbuf;
    idata->destination = dest;

    idata->source_width = width;
    idata->source_height = height;
    idata->destination_offset = offset;
    idata->aspect = aspects;
    idata->mipmap_level = mipmap_level;
    idata->layer_start = layer_start;
    idata->layer_count = layer_count;
}

void _upload_queued_buffers_cmd(VkCommandBuffer commandBuffer, void *userdata)
{
    mg::context *ctx = (mg::context *)userdata;

    for_array(sbuf, &ctx->swap_buffers)
    {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = sbuf->source->range.offset;
        copyRegion.dstOffset = sbuf->destination->range.offset;
        copyRegion.size = sbuf->size;

        trace("copying %u bytes from %p:%u to buffer %p:%u\n",
              copyRegion.size, sbuf->source, copyRegion.srcOffset,
              sbuf->destination, copyRegion.dstOffset);

        vkCmdCopyBuffer(commandBuffer, sbuf->source->buffer->buffer, sbuf->destination->buffer->buffer, 1, &copyRegion);
    }

    for_array(isbuf, &ctx->image_swap_buffers)
    {
        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset = isbuf->source->range.offset;
        copyRegion.bufferRowLength = isbuf->source_width;
        copyRegion.bufferImageHeight = isbuf->source_height;
        copyRegion.imageSubresource.aspectMask = isbuf->aspect;
        copyRegion.imageSubresource.mipLevel = isbuf->mipmap_level;
        copyRegion.imageSubresource.baseArrayLayer = isbuf->layer_start;
        copyRegion.imageSubresource.layerCount = isbuf->layer_count;
        copyRegion.imageOffset = isbuf->destination_offset;
        copyRegion.imageExtent = isbuf->destination->extent;

        trace("copying (%u x %u) from %p:%u to image %p:(%u, %u, %u)\n",
              copyRegion.bufferRowLength, copyRegion.bufferImageHeight,
              isbuf->source, copyRegion.bufferOffset,
              isbuf->destination,
              copyRegion.imageOffset.x, copyRegion.imageOffset.y, copyRegion.imageOffset.z);

        vkCmdCopyBufferToImage(commandBuffer, isbuf->source->buffer->buffer, isbuf->destination->image, isbuf->destination->layout, 1, &copyRegion);
    }
}

void mg::upload_queued_buffers(mg::context *ctx)
{
    assert(ctx != nullptr);

    if (ctx->swap_buffers.size == 0)
        return;

    mg::submit_immediate_vulkan_command_to_current_frame(ctx, _upload_queued_buffers_cmd, ctx);

    mg::clear_queued_buffers(ctx);
}

void mg::clear_queued_buffers(mg::context *ctx)
{
    assert(ctx != nullptr);

    for_array(sb, &ctx->swap_buffers)
        mg::destroy_sub_buffer(sb->source);

    ::clear(&ctx->swap_buffers);
}

bool is_surface_format_supported(mg::context *ctx)
{
    bool supported = false;
    
    array<VkSurfaceFormatKHR> formats;
    ::init(&formats);
    defer { ::free(&formats); };
    
    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physical_device, ctx->target_surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        ::resize(&formats, formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physical_device, ctx->target_surface, &formatCount, formats.data);
    }
    
#ifndef NDEBUG
    trace("surface supports %u formats:\n", formatCount);
    
    for (u32 i = 0; i < formatCount; ++i)
        trace("  %u: format = %u, colorspace = %u", i, formats[i].format, formats[i].colorSpace);
#endif // NDEBUG
    
    // make sure settings are supported
    // formats
    for_array(availableFormat, &formats)
    {
        if (availableFormat->format == ctx->config.surface.format.format
         && availableFormat->colorSpace == ctx->config.surface.format.colorSpace)
        {
            supported = true;
            break;
        }
    }
    
    return supported;
}

// sets up a vulkan instance
void mg::setup_instance(mg::context *ctx, const char **ext_names, u32 ext_count)
{
    assert(ctx != nullptr);

    array<const char*> layer_names;
    ::init(&layer_names);
    defer { for_array(layer, &layer_names) ::free_memory((char*)*layer); ::free(&layer_names); };

    array<const_string> layer_exceptions;
    ::init(&layer_exceptions, 1);
    defer { ::free(&layer_exceptions); };

    //layer_exceptions[0] = "abc"_cs;
    layer_exceptions[0] = "VK_LAYER_MESA_overlay"_cs;

    mg::get_vulkan_layer_names(&layer_names, &layer_exceptions);

    ctx->instance = ::create_vk_instance(ext_names, ext_count, layer_names.data, layer_names.size);
}

void mg::set_physical_device(mg::context *ctx)
{
    trace("setting physical device (GPU)\n");
    assert(ctx->instance != nullptr);
    
     // Get number of available physical devices, needs to be at least 1
    u32 physical_device_count = 0;
    VkResult res = vkEnumeratePhysicalDevices(ctx->instance, &physical_device_count, nullptr);
    
    if (res != VK_SUCCESS)
        throw_vk_error(res, "%p could not get physical devices", ctx);
    
    if (physical_device_count == 0)
        throw_error("%p no physical devices found", ctx);

    // Now get the devices
    array<VkPhysicalDevice> physical_devices;
    ::init(&physical_devices, physical_device_count);
    defer { ::free(&physical_devices); };

    vkEnumeratePhysicalDevices(ctx->instance, &physical_device_count, physical_devices.data);
    
    // find best GPU for graphics and set graphics queue
    VkPhysicalDevice ret = nullptr;
    u32 gqueue_index = UINT32_MAX;
    u32 max_gqueues = 0;

    array<VkQueueFamilyProperties> queue_properties;
    ::init(&queue_properties);
    defer { ::free(&queue_properties); };

    for_array(devic, &physical_devices)
    {
        u32 family_queue_count = 0;
        
        vkGetPhysicalDeviceQueueFamilyProperties(*devic, &family_queue_count, nullptr);
        
        if (family_queue_count == 0)
            continue;

        // Extract the properties of all the queue families
        ::clear(&queue_properties);
        ::reserve(&queue_properties, family_queue_count);
        
        vkGetPhysicalDeviceQueueFamilyProperties(*devic, &family_queue_count, queue_properties.data);
        queue_properties.size = family_queue_count;
        
        for_array(i, prop, &queue_properties)
        {
            trace("  Queue count: %u, Flags: %u\n", prop->queueCount, prop->queueFlags);

            if (prop->queueCount > max_gqueues
             && ((prop->queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT)
            )
            {
                ret = *devic;
                max_gqueues = prop->queueCount;
                gqueue_index = i;
            }
        }
    }
    
    if (ret == nullptr)
        throw_error("%p could not find a GPU with a graphics queue", ctx);
    
#ifndef NDEBUG
    VkPhysicalDeviceProperties chosen_props;
    vkGetPhysicalDeviceProperties(ret, &chosen_props);
    trace("Chosen GPU: %s\n", chosen_props.deviceName);
    trace("  with %d queues in the graphics queue family\n", max_gqueues);
#endif // NDEBUG
    
    ctx->graphics_queue_index = gqueue_index;
    ctx->graphics_queue_max_count = max_gqueues;
    ctx->physical_device = ret;
}

void mg::set_min_swap_image_count(mg::context *ctx, u32 default_min)
{
    trace("setting min swap image count\n");
    
    assert(ctx->physical_device != nullptr);
    assert(ctx->target_surface != nullptr);
    
    VkSurfaceCapabilitiesKHR surface_properties;
    VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physical_device, ctx->target_surface, &surface_properties);

    if (res != VK_SUCCESS)
        throw_vk_error(res, "could not get physical device surface capabilities", ctx);

    u32 min_swap_image_count = 0;
    
    if (default_min < UINT32_MAX)
        min_swap_image_count = default_min;
    else
        min_swap_image_count = surface_properties.minImageCount + 1;
    
    if (surface_properties.maxImageCount > surface_properties.minImageCount)
    {
        if (min_swap_image_count < surface_properties.minImageCount)
            min_swap_image_count = surface_properties.minImageCount;

        if (min_swap_image_count > surface_properties.maxImageCount)
            min_swap_image_count = surface_properties.maxImageCount;
    }
    else
    {
        if (min_swap_image_count < surface_properties.minImageCount)
            min_swap_image_count = surface_properties.minImageCount;
    }
    
    ctx->config.surface.min_swap_image_count = min_swap_image_count;
    trace("min swap image count set to %u\n", min_swap_image_count);
}

void mg::set_present_queue(mg::context *ctx)
{
    trace("setting presenting queue\n");
    
    assert(ctx->physical_device != nullptr);
    assert(ctx->target_surface != nullptr);
    
    VkBool32 pres = false;
    
    if (ctx->graphics_queue_index != UINT32_MAX)
    {
        VkResult res = vkGetPhysicalDeviceSurfaceSupportKHR(ctx->physical_device, ctx->graphics_queue_index, ctx->target_surface, &pres);

        if (res != VK_SUCCESS)
            throw_vk_error(res, "%p could not query physical device for surface support", ctx);
    }
    
    if (pres)
    {
        ctx->present_queue_index = ctx->graphics_queue_index;
        ctx->present_queue_max_count = ctx->graphics_queue_max_count;
        trace("presenting queue is the same as graphics queue\n");
        return;
    }
    
    u32 family_queue_count = 0;
    
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device, &family_queue_count, nullptr);
    
    if (family_queue_count == 0)
        return;

    // Extract the properties of all the queue families
    array<VkQueueFamilyProperties> queue_properties;
    ::init(&queue_properties, family_queue_count);
    defer { ::free(&queue_properties); };
    
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device, &family_queue_count, queue_properties.data);
    
    u32 max_qqueues = 0;
    u32 qqueue_index = UINT32_MAX;
    
    for_array(i, props, &queue_properties)
    {
        if (props->queueCount > max_qqueues)
        {
            pres = false;

            if (vkGetPhysicalDeviceSurfaceSupportKHR(ctx->physical_device, ctx->graphics_queue_index, ctx->target_surface, &pres) != VK_SUCCESS)
                continue;

            if (!pres)
                continue;
            
            max_qqueues = props->queueCount;
            qqueue_index = i;
        }
    }
    
    if (qqueue_index == UINT32_MAX)
        throw_error("%p could not find a presenting queue for the current GPU", ctx);
    
    ctx->present_queue_index = qqueue_index;
    ctx->present_queue_max_count = max_qqueues;
    
    trace("presenting queue index = %d, count: %d\n", qqueue_index, max_qqueues);
}

void mg::create_logical_device(mg::context *ctx, const array<const_string> *layer_exceptions)
{
    trace("creating logical device\n");
    
    assert(ctx->physical_device != nullptr);
    
    array<const char*> layers;
    ::init(&layers);
    defer { for_array(layer, &layers) ::free_memory((char*)*layer); ::free(&layers); };

    mg::get_vulkan_layer_names(&layers, layer_exceptions);
    
    // Get the number of available extensions for the graphics card
    u32 device_property_count = 0;
    
    VkResult res = vkEnumerateDeviceExtensionProperties(ctx->physical_device, NULL, &device_property_count, NULL);

    if (res != VK_SUCCESS)
        throw_vk_error(res, "%p unable to acquire device extension property count", ctx);
    
    trace("found %u device extensions", device_property_count);

    // Acquire their actual names
    array<VkExtensionProperties> device_properties;
    ::init(&device_properties, device_property_count);
    defer { ::free(&device_properties); };
    
    res = vkEnumerateDeviceExtensionProperties(ctx->physical_device, NULL, &device_property_count, device_properties.data);

    if (res != VK_SUCCESS)
        throw_vk_error(res, "%p unable to acquire device extension property names", ctx);

    // Match names against requested extension
    array<const char*> device_property_names;
    ::init(&device_property_names);
    defer { ::free(&device_property_names); };

    int count = 0;
    
    for_array(ext_property, &device_properties)
    {
        bool found = false;
        
        trace("%3i: %s\n", count, ext_property->extensionName);
        
        for_array(req_ext_name, &ctx->config.device_extension_names)
            if (compare_strings(*req_ext_name, ext_property->extensionName) == 0)
            {
                found = true;
                break;
            }
        
        if (found)
            ::add_at_end(&device_property_names, (const char*)ext_property->extensionName);
        
        count++;
    }

    assert(device_property_names.size == ctx->config.device_extension_names.size);
    
#ifdef TRACE
    for (const char *dpn : device_property_names)
        trace("applying extension %s\n", dpn);
#endif

    VkDeviceQueueCreateInfo queue_create_infos[2];
    u32 queue_count = 1;
    
    VkDeviceQueueCreateInfo *gqueue_create_info = queue_create_infos;
    gqueue_create_info->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    gqueue_create_info->queueFamilyIndex = ctx->graphics_queue_index;
    gqueue_create_info->queueCount = ctx->config.graphics_queue_count;
    gqueue_create_info->pQueuePriorities = ctx->config.graphics_queue_priorities.data;
    gqueue_create_info->pNext = nullptr;
    gqueue_create_info->flags = 0;
    
    if (ctx->present_queue_index != ctx->graphics_queue_index)
    {
        queue_count++;
        float pqueue_priority = 1.0f;

        VkDeviceQueueCreateInfo *pqueue_create_info = queue_create_infos + 1;
        pqueue_create_info->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        pqueue_create_info->queueFamilyIndex = ctx->present_queue_index;
        pqueue_create_info->queueCount = 1;
        pqueue_create_info->pQueuePriorities = &pqueue_priority;
        pqueue_create_info->pNext = nullptr;
        pqueue_create_info->flags = 0;
    }

    // Device creation information
    VkDeviceCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = queue_count;
    create_info.pQueueCreateInfos = queue_create_infos;
    create_info.ppEnabledLayerNames = layers.data;
    create_info.enabledLayerCount = static_cast<u32>(layers.size);
    create_info.ppEnabledExtensionNames = device_property_names.data;
    create_info.enabledExtensionCount = static_cast<u32>(device_property_names.size);
    create_info.pNext = nullptr;
    create_info.pEnabledFeatures = nullptr;
    create_info.flags = 0;

    // Finally we're ready to create a new device
    res = vkCreateDevice(ctx->physical_device, &create_info, nullptr, &ctx->device);
    
    if (res != VK_SUCCESS)
        throw_vk_error(res, "%p failed to create logical device", ctx);
    
    vkGetDeviceQueue(ctx->device, ctx->graphics_queue_index, 0, &ctx->graphics_queue);
    vkGetDeviceQueue(ctx->device, ctx->present_queue_index, 0, &ctx->present_queue);
    
    trace("logical device created successfully\n");
}

void mg::create_swapchain(mg::context *ctx)
{
    trace("creating swapchain\n");
    
    assert(ctx->physical_device != nullptr);
    assert(ctx->target_surface != nullptr);
    
    VkPresentModeKHR present_mode = mg::get_supported_present_mode(ctx);
    
    if (!::is_surface_format_supported(ctx))
        throw_error("%p surface format %u with colorspace %u is not supported", ctx, ctx->config.surface.format.format, ctx->config.surface.format.colorSpace);

    if (present_mode == VK_PRESENT_MODE_MAX_ENUM_KHR)
        throw_error("%p neither present mode %u nor fallback mode %u are supported", ctx, ctx->config.swap.present_mode, ctx->config.swap.fallback_present_mode);
    
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physical_device, ctx->target_surface, &capabilities);
    
    if (ctx->config.swap.transform == VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR)
        ctx->config.swap.transform = capabilities.currentTransform;
    
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = ctx->target_surface;
    
    createInfo.minImageCount = ctx->config.surface.min_swap_image_count;
    createInfo.imageFormat = ctx->config.surface.format.format;
    createInfo.imageColorSpace = ctx->config.surface.format.colorSpace;
    createInfo.imageExtent = ctx->config.surface.extent;
    createInfo.imageArrayLayers = 1; // only for stereoscopic 3D
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    u32 queueFamilyIndices[] = {
        ctx->graphics_queue_index,
        ctx->present_queue_index
    };
    
    if (queueFamilyIndices[0] != queueFamilyIndices[1])
    {
        trace("using 2 concurrent queues\n");
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        trace("using 1 exclusive queue\n");
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    
    createInfo.preTransform = ctx->config.swap.transform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = present_mode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = ctx->swapchain;

    VkResult res = vkCreateSwapchainKHR(ctx->device, &createInfo, nullptr, &ctx->swapchain);
    
    if (res != VK_SUCCESS)
        throw_vk_error(res, "%p failed to create swap chain", ctx);
    
    trace("swapchain created successfully\n");
}

void mg::set_swapchain_images(mg::context *ctx)
{
    trace("setting swapchain images\n");

    assert(ctx->device != nullptr);
    assert(ctx->swapchain != nullptr);
    
    u32 image_count;
    vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &image_count, nullptr);
    ::resize(&ctx->swapchain_images, image_count);
    vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &image_count, ctx->swapchain_images.data);

    ::resize(&ctx->swapchain_image_fences, image_count);

    for (u32 i = 0; i < image_count; ++i)
        ctx->swapchain_image_fences[i] = nullptr;
}

void mg::create_swapchain_image_views(mg::context *ctx)
{
    trace("creating image views\n");

    assert(ctx->device != nullptr);
    
    ::resize(&ctx->swapchain_image_views, ctx->swapchain_images.size);
    
    for_array(i, img, &ctx->swapchain_images)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = *img;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = ctx->config.surface.format.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult res = vkCreateImageView(ctx->device, &createInfo, nullptr, ctx->swapchain_image_views.data + i);

        if (res != VK_SUCCESS)
            throw_vk_error(res, "%p failed to create image views", ctx);
    }
    
    trace("image views created successfully\n");
}

void mg::create_render_pass(mg::context *ctx)
{
    trace("creating render pass\n");
    assert(ctx->device != nullptr);

    VkAttachmentDescription attachments[1];
    
    VkAttachmentDescription *colorAttachment = &attachments[0];
    colorAttachment->flags = 0;
    colorAttachment->format = ctx->config.surface.format.format;
    colorAttachment->samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment->finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = nullptr;

    VkSubpassDependency dependencies[1];

    VkSubpassDependency *dependency = &dependencies[0];
    dependency->dependencyFlags = 0;
	dependency->srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency->dstSubpass = 0;
	dependency->srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency->srcAccessMask = 0;
	dependency->dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency->dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = dependencies;
    
    VkResult res = vkCreateRenderPass(ctx->device, &renderPassInfo, nullptr, &ctx->render_pass);

    if (res != VK_SUCCESS)
        throw_vk_error(res, "%p failed to create render pass", ctx);
    
    trace("render pass created successfully\n");
}

void mg::create_framebuffers(mg::context *ctx)
{
    trace("creating framebuffers\n");
    assert(ctx->device != nullptr);
    assert(ctx->render_pass != nullptr);
    
    ::resize(&ctx->framebuffers, ctx->swapchain_image_views.size);
    
    for_array(i, iv, &ctx->swapchain_image_views)
    {
        VkImageView attachments[] = {
            *iv
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = ctx->render_pass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = ctx->config.surface.extent.width;
        framebufferInfo.height = ctx->config.surface.extent.height;
        framebufferInfo.layers = 1;
        
        VkResult res = vkCreateFramebuffer(ctx->device, &framebufferInfo, nullptr, &ctx->framebuffers[i]);

        if (res != VK_SUCCESS)
            throw_vk_error(res, "%p failed to create framebuffer", ctx);
    }
    
    trace("framebuffers created successfully\n");
}

void mg::create_frame_data(mg::context *ctx)
{
    trace("creating frame data\n");
    assert(ctx->device != nullptr);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = ctx->graphics_queue_index;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        mg::frame_data *frame = &ctx->frames[i];
        
        VkResult res = vkCreateCommandPool(ctx->device, &poolInfo, nullptr, &frame->command_pool);

        if (res != VK_SUCCESS)
            throw_vk_error(res, "%p failed to create command pool", ctx);

        ::resize(&frame->command_buffers, ctx->framebuffers.size);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = frame->command_pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(frame->command_buffers.size);

        res = vkAllocateCommandBuffers(ctx->device, &allocInfo, frame->command_buffers.data);

        if (res != VK_SUCCESS)
            throw_vk_error(res, "%p failed to allocate command buffers", ctx);

        VkSemaphoreCreateInfo semaphoreInfo;
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreInfo.pNext = nullptr;
        semaphoreInfo.flags = 0;
        
        res = vkCreateSemaphore(ctx->device, &semaphoreInfo, nullptr, &frame->render_semaphore);

        if (res != VK_SUCCESS)
            throw_vk_error(res, "%p failed to create render semaphore", ctx);

        res = vkCreateSemaphore(ctx->device, &semaphoreInfo, nullptr, &frame->present_semaphore);

        if (res != VK_SUCCESS)
            throw_vk_error(res, "%p failed to create present semaphore", ctx);

        VkFenceCreateInfo fenceInfo;
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.pNext = nullptr;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        
        res = vkCreateFence(ctx->device, &fenceInfo, nullptr, &frame->render_fence);

        if (res != VK_SUCCESS)
            throw_vk_error(res, "%p failed to create fence", ctx);

        mg::init(&frame->descriptor_pool_manager, ctx);
    }
}

// =======
// DESTROY
// =======
void mg::destroy_frame_data(mg::context *ctx)
{
    trace("destroying frame data\n");
    assert(ctx->device != nullptr);
    
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        mg::frame_data *frame = &ctx->frames[i];

        vkDestroyFence(ctx->device, frame->render_fence, nullptr);
        vkDestroySemaphore(ctx->device, frame->present_semaphore, nullptr);
        vkDestroySemaphore(ctx->device, frame->render_semaphore, nullptr);
        vkDestroyCommandPool(ctx->device, frame->command_pool, nullptr);

        ::free(&frame->command_buffers);
        mg::free(&frame->descriptor_pool_manager);
    }
}

void mg::destroy_framebuffers(mg::context *ctx)
{
    trace("destroying framebuffers\n");
    assert(ctx->device != nullptr);
    
    for_array(fb, &ctx->framebuffers)
        vkDestroyFramebuffer(ctx->device, *fb, nullptr);
    
    ::clear(&ctx->framebuffers);
}

void mg::destroy_render_pass(mg::context *ctx)
{
    trace("destroying render pass\n");
    assert(ctx->device != nullptr);
    
    if (ctx->render_pass == nullptr)
        return;
    
    vkDestroyRenderPass(ctx->device, ctx->render_pass, nullptr);
    
    ctx->render_pass = nullptr;
}

void mg::destroy_swapchain_image_views(mg::context *ctx)
{
    trace("destroying image views\n");
    assert(ctx->device != nullptr);
    
    for_array(iv, &ctx->swapchain_image_views)
        vkDestroyImageView(ctx->device, *iv, nullptr);
    
    ::clear(&ctx->swapchain_image_views);
}

void mg::destroy_swapchain(mg::context *ctx)
{
    trace("destroying swapchain\n");
    assert(ctx->device != nullptr);
    
    if (ctx->swapchain == nullptr)
        return;
    
    vkDestroySwapchainKHR(ctx->device, ctx->swapchain, nullptr);
    ctx->swapchain = nullptr;
}

void mg::destroy_target_surface(mg::context *ctx)
{
    trace("destroying target surface\n");
    assert(ctx->instance != nullptr);
    
    if (ctx->target_surface == nullptr)
        return;
    
    vkDestroySurfaceKHR(ctx->instance, ctx->target_surface, nullptr);
    ctx->target_surface = nullptr;
}

void mg::destroy_descriptor_pool_manager(mg::context *ctx)
{
    trace("destroying the global descriptor set manager\n");

    mg::free(&ctx->descriptor_pool_manager);
}

void mg::destroy_memory_manager(mg::context *ctx)
{
    trace("destroying memory manager\n");

    mg::free(&ctx->memory_manager);
}

void mg::destroy_logical_device(mg::context *ctx)
{
    trace("destroying logical device\n");

    if (ctx->device == nullptr)
        return;
    
    vkDestroyDevice(ctx->device, nullptr);
    ctx->device = nullptr;
}

void mg::destroy_vulkan_instance(mg::context *ctx)
{
    trace("destroying vulkan instance\n");

    if (ctx->instance == nullptr)
        return;
    
    vkDestroyInstance(ctx->instance, nullptr);
    ctx->instance = nullptr;
}

void mg::free(mg::context *ctx)
{
    if (ctx->graphics_queue != nullptr)
        vkQueueWaitIdle(ctx->graphics_queue);

    if (ctx->present_queue != nullptr)
        vkQueueWaitIdle(ctx->present_queue);

    destroy_frame_data(ctx);

    destroy_framebuffers(ctx);
    destroy_render_pass(ctx);
    destroy_swapchain_image_views(ctx);
    destroy_swapchain(ctx);
    ::clear(&ctx->swapchain_images);
    destroy_target_surface(ctx);
    clear_queued_buffers(ctx);
    destroy_descriptor_pool_manager(ctx);
    destroy_memory_manager(ctx);
    destroy_logical_device(ctx);
    destroy_vulkan_instance(ctx);

    ::free(&ctx->swap_buffers);
    ::free(&ctx->image_swap_buffers);

    ::free(&ctx->swapchain_images);
    ::free(&ctx->swapchain_image_views);
    ::free(&ctx->swapchain_image_fences);

    ::free(&ctx->framebuffers);

    mg::free(&ctx->config);
}
