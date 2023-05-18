
#include "mg/mg.hpp"

constexpr const char *DEMO_NAME = "ui_demo";
int window_width = 640;
int window_height = 480;

void update(mg::window *window, double dt)
{
    ui::new_frame(window);
    ImGui::ShowDemoWindow();
    ui::end_frame();
}

int main(int argc, const char *argv[])
{
    mg::window window;
    mg::create_window(&window, DEMO_NAME, window_width, window_height);

    mg::event_loop(&window, ::update);

    mg::destroy_window(&window);

    return 0;
}
