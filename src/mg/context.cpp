
#include <assert.h>

#include "shl/number_types.hpp"
#include "shl/memory.hpp"
#include "shl/debug.hpp"
#include "shl/defer.hpp"

#include "mg/impl/context.hpp"
#include "mg/impl/window.hpp"

#include "mg/vk_error.hpp"
#include "mg/context.hpp"

template<typename T>
T clamp(T &&a, T &&low, T &&high)
{
    if (a < low)
        return low;
    else if (a > high)
        return high;

    return a;
}

mg::context *mg::create_context()
{
    mg::context *ctx = ::allocate_memory<mg::context>();
    mg::init(ctx);
    return ctx;
}

void mg::destroy_context(mg::context *ctx)
{
    mg::free(ctx);
    ::free_memory(ctx);
}

void mg::setup_vulkan_instance(context *ctx, mg::window *window)
{
    assert(ctx != nullptr);

    array<const char*> ext_names;
    ::init(&ext_names);
    defer { ::free(&ext_names); };

    u32 ext_count = mg::get_vulkan_instance_extensions(&ext_names, window);

    mg::setup_instance(ctx, ext_names.data, ext_count);
}

void mg::create_vulkan_surface(context *ctx, mg::window *window)
{
    assert(ctx != nullptr);
    assert(window != nullptr);
    
    VkResult res = mg::create_vulkan_surface(window, ctx->instance, &ctx->target_surface);

    if (res != VK_SUCCESS)
        throw_vk_error(res, "%p could not create vulkan surface for window %p", ctx, window);
}

void mg::setup_window_context(context *ctx, mg::window *window)
{
    assert(ctx != nullptr);
    assert(window != nullptr);

    ctx->window = window;

    mg::set_physical_device(ctx);
    mg::set_min_swap_image_count(ctx, UINT32_MAX);

    int w;
    int h;
    mg::get_window_size(window, &w, &h);

    ctx->config.viewport.x = 0;
    ctx->config.viewport.y = 0;
    ctx->config.viewport.minDepth = 0.0f;
    ctx->config.viewport.maxDepth = 1.0f;

    ctx->config.scissor.offset = {0, 0};

    mg::set_render_size(ctx, w, h);

    mg::set_present_queue(ctx);
    mg::create_logical_device(ctx);
    mg::init(&ctx->memory_manager, ctx);
    mg::init(&ctx->descriptor_pool_manager, ctx);
    mg::create_swapchain(ctx);
    mg::set_swapchain_images(ctx);
    mg::create_swapchain_image_views(ctx);
    mg::create_render_pass(ctx);
    mg::create_framebuffers(ctx);
    mg::create_frame_data(ctx);
}

void mg::set_render_size(context *ctx, u32 width, u32 height)
{
    trace("setting image extent");
    assert(ctx->physical_device != nullptr);
    assert(ctx->target_surface != nullptr);
    
    VkSurfaceCapabilitiesKHR capabilities;

    VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physical_device, ctx->target_surface, &capabilities);
    
    if (res != VK_SUCCESS)
        throw_vk_error(res, "%p could not obtain surface capabilities", ctx);
    
    VkExtent2D size;
    
    // This happens when the window scales based on the size of an image
    if (capabilities.currentExtent.width == 0xFFFFFFFu)
    {
        VkExtent2D windowsize{width, height};
        
        size.width  = clamp(windowsize.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
        size.height = clamp(windowsize.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    else
        size = capabilities.currentExtent;
    
    trace("setting image extent to %u x %u", size.width, size.height);
    ctx->config.surface.extent = size;

    ctx->config.viewport.width = size.width;
    ctx->config.viewport.height = size.height;

    ctx->config.scissor.extent = size;

    ctx->changed.extent = true;
}

void mg::get_time_data(mg::context *ctx, mg::time_data **td)
{
    assert(ctx != nullptr);
    assert(td != nullptr);

    *td = &ctx->time_data;
}

void submit_frame_commands(mg::context *ctx, mg::frame_data *frame, u32 image_index)
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &frame->present_semaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &frame->render_semaphore;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frame->command_buffers[image_index];

    VkResult res = vkQueueSubmit(ctx->graphics_queue, 1, &submitInfo, frame->render_fence);

    if (res != VK_SUCCESS)
        throw_vk_error(res, "%p failed to submit draw command buffer", ctx);
}

void recreate_swapchain(mg::context *ctx)
{
    vkDeviceWaitIdle(ctx->device);
    

    mg::destroy_frame_data(ctx);
    mg::destroy_framebuffers(ctx);
    mg::destroy_render_pass(ctx);
    mg::destroy_swapchain_image_views(ctx);

    mg::create_swapchain(ctx);
    mg::set_swapchain_images(ctx);
    mg::create_swapchain_image_views(ctx);
    mg::create_render_pass(ctx);
    mg::create_framebuffers(ctx);
    mg::create_frame_data(ctx);
}

void present_frame(mg::context *ctx, mg::frame_data *frame, u32 image_index)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &frame->render_semaphore;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &ctx->swapchain;

    presentInfo.pImageIndices = &image_index;

    VkResult res = vkQueuePresentKHR(ctx->present_queue, &presentInfo);
    
    if (res == VK_ERROR_OUT_OF_DATE_KHR 
     || res == VK_SUBOPTIMAL_KHR
     || ctx->changed.extent)
    {
        ctx->changed.extent = false;
        ::recreate_swapchain(ctx);
    }
    else if (res != VK_SUCCESS)
        throw_vk_error(res, "failed to present swap chain image", ctx);
}

void mg::update(mg::context *ctx, float dt)
{
    ctx->time_data.elapsed_time += dt;

    mg::upload_queued_buffers(ctx);
}

bool mg::start_rendering(mg::context *ctx)
{
    VkResult res;
    mg::frame_data *frame = &ctx->frames[ctx->current_frame];

    vkWaitForFences(ctx->device, 1, &frame->render_fence, true, UINT64_MAX);
    mg::descriptor_pool_manager *descriptor_mgr = &frame->descriptor_pool_manager;
    mg::reset_pools(descriptor_mgr);

    res = vkAcquireNextImageKHR(ctx->device, ctx->swapchain, UINT64_MAX, frame->present_semaphore, nullptr, &ctx->current_image_index);
    u32 image_index = ctx->current_image_index;

    if (res == VK_ERROR_OUT_OF_DATE_KHR
     || ctx->changed.extent)
    {
        ctx->changed.extent = false;
        ::recreate_swapchain(ctx);
        return false;
    }

    if (ctx->swapchain_image_fences[image_index] != nullptr)
        vkWaitForFences(ctx->device, 1, &ctx->swapchain_image_fences[image_index], VK_TRUE, UINT64_MAX);

    ctx->swapchain_image_fences[image_index] = frame->render_fence;

    VkCommandBufferBeginInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.pInheritanceInfo = nullptr;
	bufferInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkCommandBuffer buf = frame->command_buffers[image_index];
    
    vkResetCommandBuffer(buf, 0);
    res = vkBeginCommandBuffer(buf, &bufferInfo);
    
    if (res != VK_SUCCESS)
        throw_vk_error(res, "%p could not begin command buffer", ctx);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = ctx->render_pass;
    renderPassInfo.framebuffer = ctx->framebuffers[image_index];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = ctx->config.surface.extent;

    VkClearValue clearValues[] = {
        {.color{{0.0f, 0.0f, 0.0f, 1.0f}}},
        {.depthStencil{1.f, 0}}
    };
    
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(buf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    return true;
}

void mg::end_rendering(mg::context *ctx)
{
    mg::frame_data *frame = ctx->frames + ctx->current_frame;
    u32 image_index = ctx->current_image_index;
    VkCommandBuffer buf = frame->command_buffers[image_index];

    vkCmdEndRenderPass(buf);
    vkEndCommandBuffer(buf);

    vkResetFences(ctx->device, 1, &frame->render_fence);
    ::submit_frame_commands(ctx, frame, image_index);
}

void mg::present(mg::context *ctx)
{
    ::present_frame(ctx, ctx->frames + ctx->current_frame, ctx->current_image_index);
    
    ctx->current_frame = (ctx->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    ctx->time_data.total_frame_count += 1;
}
