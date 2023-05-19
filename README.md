# mg
Small utility library to get a window with imgui running quickly, based on a private vulkan library.

The point of this library is to quickly get a window going in which you can use [ImGui](https://github.com/ocornut/imgui) in, regardless of the platform you're on.

`mg` uses Vulkan and either SDL or GLFW (set CMake Variable `WINDOW` to either `SDL` or `GLFW`, the default is GLFW) and doesn't hide the details from you in case you need to access the underlying data structures.

## Example

[(main.cpp from the demo)](/demos/ui_demo/src/main.cpp)
```cpp
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
```

Which creates a resizable window with the ImGui demo window inside.

![image](https://github.com/DaemonTsun/mg/assets/96687758/9765a554-0704-442f-abdb-a240cd562763)

## Dependencies

- `libvulkan 1.2.0+`
- `libsdl2` or `libglfw3` (GLFW does not need to be installed, if GLFW is selected the CMake project builds GLFW statically because it is included as a git submodule)
- `libimgui` (included as a git submodule, does not need to be installed)

## Building

If submodules are not initialized, initialize them with `git submodule update --init --recursive`.

```sh
$ mkdir bin
$ cd bin
$ cmake ..
$ make
```

## How to include

Ideally use CMake, clone the repository or add it to your project as a submodule, then add the following to your CMakeLists.txt (adjust to the mg version youre using):

```cmake
add_subdirectory(path/to/mg)
target_link_libraries(your-target PRIVATE mg-0.8)
target_include_directories(your-target PRIVATE ${mg-0.8_SOURCES_DIR} path/to/mg/ext/imgui)
```

OR using [better-cmake](https://github.com/DaemonTsun/better-cmake), add the following to your targets external libs:

```cmake
LIB mg  0.8.0 "path/to/mg" INCLUDE LINK
```
