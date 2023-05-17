
#pragma once

#include "shl/number_types.hpp"

#include "mg/window.hpp"

namespace mg
{
struct scene;
struct context;

struct time_data
{
    float elapsed_time;
    u64 total_frame_count;
};

mg::context *create_context();
void destroy_context(mg::context *ctx);

// window parameter not needed with GLFW
void setup_vulkan_instance(mg::context *ctx, mg::window *window = nullptr);
void create_vulkan_surface(mg::context *ctx, mg::window *window);
void setup_window_context(mg::context *ctx, mg::window *window);
void set_render_size(mg::context *ctx, u32 width, u32 height);

void get_time_data(mg::context *ctx, time_data **td);

// dt only used for keeping track of time
void update(mg::context *ctx, float dt = 0.f);
bool start_rendering(mg::context *ctx);
void end_rendering(mg::context *ctx);
void present(mg::context *ctx);
}
