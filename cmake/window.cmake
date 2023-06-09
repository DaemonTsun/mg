
if (NOT DEFINED WINDOW)
    set(WINDOW GLFW)
endif()

if (WINDOW STREQUAL "SDL")

    find_package(SDL2 REQUIRED)
    set(WINDOW_LIBRARIES "${SDL2_LIBRARIES}")
    set(WINDOW_INCLUDE_DIRS "${SDL2_INCLUDE_DIRS}")
    set(WINDOW_COMPILE_DEFINITIONS MG_USE_SDL)

elseif (WINDOW STREQUAL "GLFW")
    set(GLFW_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw")
    if (NOT EXISTS "${GLFW_PATH}/CMakeLists.txt")
        execute_process(COMMAND git submodule update --init "${GLFW_PATH}"
                        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    set(GLFW_LIBRARY_TYPE STATIC)
    set(GLFW_BUILD_DOCS FALSE)
    # set(GLFW_VULKAN_STATIC ON)
    add_subdirectory("${GLFW_PATH}")

    if (NOT DEFINED GLFW_LIBRARIES)
        set(GLFW_LIBRARIES glfw)
    endif()

    set(WINDOW_LIBRARIES "${GLFW_LIBRARIES}")
    set(WINDOW_INCLUDE_DIRS "${GLFW_PATH}/include")
    set(WINDOW_COMPILE_DEFINITIONS MG_USE_GLFW)
endif()
