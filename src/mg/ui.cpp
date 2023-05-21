
#include <vulkan/vulkan_core.h>

#include "backends/imgui_impl_vulkan.h"

#if defined MG_USE_SDL
#include "backends/imgui_impl_sdl2.h"
#elif defined MG_USE_GLFW
#include "backends/imgui_impl_glfw.h"
#endif

#include "mg/impl/context.hpp"
#include "mg/impl/window.hpp"
#include "mg/vk_error.hpp"
#include "mg/ui.hpp"

void _imgui_create_fonts_texture(VkCommandBuffer buf, void *)
{
    ImGui_ImplVulkan_CreateFontsTexture(buf);
}

void ui::init(mg::window *window)
{
    mg::context *ctx = window->context;

    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = 11;
    pool_info.pPoolSizes = pool_sizes;

    VkDescriptorPool imguiPool;
    mg::create_descriptor_pool(&ctx->descriptor_pool_manager, &pool_info, &imguiPool);

    // create imgui
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

#if defined MG_USE_SDL
    ImGui_ImplSDL2_InitForVulkan(window->handle);
#elif defined MG_USE_GLFW
    // we hook events later
    ImGui_ImplGlfw_InitForVulkan(window->handle, false);
#endif

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = ctx->instance;
    init_info.PhysicalDevice = ctx->physical_device;
    init_info.Device = ctx->device;
    init_info.Queue = ctx->graphics_queue;
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info, ctx->render_pass);

    // upload fonts, then delete them from cpu
    mg::submit_immediate_vulkan_command_to_current_frame(ctx, ::_imgui_create_fonts_texture, nullptr);
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void ui::exit(mg::window *window)
{
    mg::context *ctx = window->context;

    vkDeviceWaitIdle(ctx->device);
    ImGui_ImplVulkan_Shutdown();
#if defined MG_USE_SDL
    ImGui_ImplSDL2_Shutdown();
#elif defined MG_USE_GLFW
    ImGui_ImplGlfw_Shutdown();
#endif
    ImGui::DestroyContext();
}

void ui::new_frame(mg::window *window)
{
    ImGui_ImplVulkan_NewFrame();

#if defined MG_USE_SDL
    ImGui_ImplSDL2_NewFrame(window->handle);
#elif defined MG_USE_GLFW
    ImGui_ImplGlfw_NewFrame();
#endif

    ImGui::NewFrame();
}

void ui::end_frame()
{
    ImGui::EndFrame();
}

#if defined MG_USE_SDL
void _imgui_sdl_event_callback(mg::window *window, void *_e)
{
    SDL_Event *e = (SDL_Event*)_e;
    ImGui_ImplSDL2_ProcessEvent(e);
}
#endif

void ui::set_window_ui_callbacks(mg::window *window)
{
#if defined MG_USE_SDL
    mg::add_window_event_handler(window, ::_imgui_sdl_event_callback);
    
#elif defined MG_USE_GLFW
    ImGui_ImplGlfw_InstallCallbacks(window->handle);
#endif
}

void ui::render(mg::window *window)
{
    mg::context *ctx = window->context;

    mg::frame_data *frame = ctx->frames + ctx->current_frame;
    VkCommandBuffer buf = frame->command_buffers[ctx->current_image_index];

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buf);
}
