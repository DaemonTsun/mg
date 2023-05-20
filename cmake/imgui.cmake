
set(imgui_SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/")
set(imgui_SOURCES 
    "${imgui_SOURCES_DIR}/imgui.h"
    "${imgui_SOURCES_DIR}/imgui.cpp"

#   "${imgui_SOURCES_DIR}/imgui_demo.cpp"
    "${imgui_SOURCES_DIR}/imgui_draw.cpp"
    "${imgui_SOURCES_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCES_DIR}/imgui_tables.cpp"

    "${imgui_SOURCES_DIR}/backends/imgui_impl_vulkan.cpp"
)

if (WINDOW STREQUAL "SDL")
    set(imgui_SOURCES ${imgui_SOURCES} "${imgui_SOURCES_DIR}/backends/imgui_impl_sdl2.cpp")
elseif (WINDOW STREQUAL "GLFW")
    set(imgui_SOURCES ${imgui_SOURCES} "${imgui_SOURCES_DIR}/backends/imgui_impl_glfw.cpp")
endif()
    
set(imgui_INCLUDE_DIRECTORIES "${imgui_SOURCES_DIR}")
