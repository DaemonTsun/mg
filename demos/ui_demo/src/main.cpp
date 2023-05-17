
#include "mg/mg.hpp"

constexpr const char *DEMO_NAME = "ui_demo";

int window_width = 640;
int window_height = 480;

mg::context *ctx = nullptr;
mg::window *window = nullptr;

void setup()
{
    window = mg::create_window(DEMO_NAME, window_width, window_height);

    ctx = mg::create_context();
    mg::setup_vulkan_instance(ctx, window);
    mg::create_vulkan_surface(ctx, window);
    mg::setup_window_context(ctx, window);

    ui::init(ctx);
}

void cleanup()
{
    ui::exit(ctx);
    mg::destroy_context(ctx);
    mg::destroy_window(window);
    mg::quit_windowing_system();
}

void update(double dt)
{
    mg::update(ctx);

    ui::new_frame(ctx);
    // TODO: get rid of demo
    ImGui::ShowDemoWindow();
    ui::end_frame();
}

void render(double dt)
{
    // e.g. if window size changed, exit
    if (!mg::start_rendering(ctx))
        return;

    ui::render(ctx);
    mg::end_rendering(ctx);

    mg::present(ctx);
}

void on_window_resize(mg::window *window, int width, int height)
{
    mg::set_render_size(ctx, width, height);
}

void event_loop()
{
    mg::set_window_resize_callback(window, ::on_window_resize);
    mg::set_window_resize_timeout(0.1);

    mg::set_event_loop_update_callback(::update);
    mg::set_event_loop_render_callback(::render);

    // should be last
    ui::set_window_ui_callbacks(window);

    mg::event_loop(window);
}

int main(int argc, const char *argv[])
{
    ::setup();
    ::event_loop();
    ::cleanup();

    return 0;
}
