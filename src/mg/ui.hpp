
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

void init(mg::window *window);
void exit(mg::window *window);

// just convenience
void new_frame(mg::window *window);
void end_frame();

void set_window_ui_callbacks(mg::window *window);

void render(mg::window *window);
}
