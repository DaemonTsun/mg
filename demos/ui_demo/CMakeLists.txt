cmake_minimum_required(VERSION 3.20)

find_package(better REQUIRED NO_DEFAULT_PATH PATHS ../../ext/better-cmake/cmake/)

add_exe(ui_demo
    VERSION 1.0.0
    SOURCES_DIR "${ROOT}/src"
    SOURCES "${imgui_SOURCES_DIR}/imgui_demo.cpp"
    CPP_VERSION 17
    CPP_WARNINGS ALL SANE FATAL
    LIBRARIES ${mg_TARGET}
    INCLUDE_DIRS "${mg_SOURCES_DIR}" "${mg_INCLUDE_DIRECTORIES}"
    )

# run
add_custom_target("run_${ui_demo_TARGET}" COMMAND "${ROOT_BIN}/${ui_demo_TARGET}")
