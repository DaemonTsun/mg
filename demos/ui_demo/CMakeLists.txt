cmake_minimum_required(VERSION 3.20)

find_package(better REQUIRED NO_DEFAULT_PATH PATHS ../../ext/better-cmake/cmake/)

add_exe(ui_demo
    SOURCES_DIR "${ROOT}/src"
    SOURCES "${imgui_SOURCES_DIR}/imgui_demo.cpp"
    CPP_VERSION 17
    CPP_WARNINGS ALL SANE FATAL
    LIBRARIES ${mg_TARGET}
    INCLUDE_DIRS "${mg_SOURCES_DIR}" "${mg_INCLUDE_DIRS}"
    )

# run
add_custom_target("run_ui_demo" COMMAND "${ROOT_BIN}/ui_demo")
