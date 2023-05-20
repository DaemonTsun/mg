cmake_minimum_required(VERSION 3.20)
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ext/better-cmake/cmake/betterConfig.cmake")
    execute_process(COMMAND git submodule update --init "${CMAKE_CURRENT_SOURCE_DIR}/ext/better-cmake" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}") 
endif()
find_package(better REQUIRED NO_DEFAULT_PATH PATHS ext/better-cmake/cmake)

project(mg)
project_author("DaemonTsun")
# add_definitions(-DTRACE)

find_package(Vulkan REQUIRED)

# simply set the WINDOW variable to SDL or GLFW to choose windowing library
include("cmake/window.cmake")

# includes the necessary imgui source files with the used windowing system
include("cmake/imgui.cmake")

add_lib(mg STATIC
    VERSION 0.8.0
    SOURCES_DIR "${ROOT}/src/"
    GENERATE_TARGET_HEADER "${ROOT}/src/mg/mg_info.hpp"
    CPP_VERSION 17
    CPP_WARNINGS ALL SANE FATAL
    LIBRARIES ${WINDOW_LIBRARIES} ${Vulkan_LIBRARIES}
    INCLUDE_DIRS ${WINDOW_INCLUDE_DIRS}
                 ${imgui_INCLUDE_DIRS}
    SOURCES ${imgui_SOURCES}

    EXT
        LIB shl 0.7.0 "${ROOT}/ext/shl" INCLUDE LINK GIT_SUBMODULE
    )

target_compile_definitions(mg-0.8 PUBLIC ${WINDOW_COMPILE_DEFINITIONS})

exit_if_included()

set(shl_SOURCES_DIR "${shl-0.7.0_SOURCES_DIR}")

add_subdirectory("${ROOT}/demos/ui_demo")

