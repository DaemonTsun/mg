
if (NOT DEFINED WINDOW)
    set(WINDOW GLFW)
endif()

if (WINDOW STREQUAL "SDL")
    add_compile_definitions(MG_USE_SDL)

    find_package(SDL2 REQUIRED)
    set(WINDOW_LIBRARIES "${SDL2_LIBRARIES}")
    set(WINDOW_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}")

elseif (WINDOW STREQUAL "GLFW")
    add_compile_definitions(MG_USE_GLFW)

    set(GLFW_PATH "${ROOT}/ext/glfw")

    set(GLFW_LIBRARY_TYPE STATIC)
    set(GLFW_BUILD_DOCS FALSE)
    # set(GLFW_VULKAN_STATIC ON)
    add_subdirectory("${GLFW_PATH}")

    if (NOT DEFINED GLFW_LIBRARIES)
        set(GLFW_LIBRARIES glfw)
    endif()

    set(WINDOW_LIBRARIES "${GLFW_LIBRARIES}")
    set(WINDOW_INCLUDE_DIRECTORIES "${GLFW_PATH}/include")
endif()
