
#pragma once

#include "imgui.h"
#include "mg/window.hpp"

namespace mg
{
struct context;
}

namespace ui
{
using namespace ImGui;

void init(mg::context *ctx);
void exit(mg::context *ctx);

// just convenience
void new_frame(mg::context *ctx);
void end_frame();

void set_window_ui_callbacks(mg::window *window);

void render(mg::context *ctx);
}
