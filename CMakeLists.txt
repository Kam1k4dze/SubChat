cmake_minimum_required(VERSION 3.14)
project(SubChat LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif ()


option(BUILD_GUI "Build the GUI config generator" ON)

# External headers common to both targets
set(TINYXML_DIR "${CMAKE_SOURCE_DIR}/submodules/tinyxml2")
set(SIMPLEINI_DIR "${CMAKE_SOURCE_DIR}/submodules/simpleini")
set(MAGICENUM_DIR "${CMAKE_SOURCE_DIR}/submodules/magic_enum")
set(UTFCPP_DIR "${CMAKE_SOURCE_DIR}/submodules/utfcpp/")
include_directories(
        ${TINYXML_DIR}
        ${SIMPLEINI_DIR}
        ${MAGICENUM_DIR}/include/magic_enum
        ${UTFCPP_DIR}/source
)

# ─────────────────────────────────────────────────────────────────
# Static executable for subtitle generation (CLI)
# ─────────────────────────────────────────────────────────────────
set(CSV_CXX_STANDARD ${CMAKE_CXX_STANDARD})
# Removed CLI11 dependency

add_executable(subtitles_generator
        cli_main.cpp
        ${TINYXML_DIR}/tinyxml2.cpp
)
target_include_directories(subtitles_generator
        PUBLIC
        ${TINYXML_DIR}
        ${SIMPLEINI_DIR}
        ${UTFCPP_DIR}/source
        ${MAGICENUM_DIR}/include/magic_enum
)
target_link_options(subtitles_generator PRIVATE -static)
# Removed CLI11 from target_link_libraries

# ─────────────────────────────────────────────────────────────────
# GUI config generator
# ─────────────────────────────────────────────────────────────────
if (BUILD_GUI)
    # Find GUI dependencies
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)

    # GLFW configuration
    set(GLFW_DIR "${CMAKE_SOURCE_DIR}/submodules/glfw")
    option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
    option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
    option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
    option(GLFW_INSTALL "Generate installation target" OFF)
    option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
    add_subdirectory(${GLFW_DIR} ${CMAKE_BINARY_DIR}/glfw EXCLUDE_FROM_ALL)
    include_directories(${GLFW_DIR}/include ${GLFW_DIR}/deps)

    # Dear ImGui configuration
    set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/submodules/imgui")
    include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends)

    # Add Native File Dialog
    add_subdirectory("${CMAKE_SOURCE_DIR}/submodules/nativefiledialog-extended")

    # Add GUI executable
    add_executable(config_generator_gui
            ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
            ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
            ${IMGUI_DIR}/imgui.cpp
            ${IMGUI_DIR}/imgui_draw.cpp
            ${IMGUI_DIR}/imgui_demo.cpp
            ${IMGUI_DIR}/imgui_tables.cpp
            ${IMGUI_DIR}/imgui_widgets.cpp
            ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
            ${TINYXML_DIR}/tinyxml2.cpp
            gui_main.cpp
            fonts/lucon.hpp
    )

    target_link_libraries(config_generator_gui
            PRIVATE
            glfw
            OpenGL::GL
            GLEW::GLEW
            nfd
    )
endif ()