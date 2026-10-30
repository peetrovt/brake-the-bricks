cmake_minimum_required(VERSION 3.10)
project(brake_the_bricks CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL QUIET)
find_package(glfw3 3.3 QUIET)
if(NOT OPENGL_FOUND OR NOT glfw3_FOUND)
    message(FATAL_ERROR
        "Brake The Bricks needs the OpenGL and GLFW development files (headers and libraries).\n"
        "On Ubuntu/Debian install them with:\n"
        "    sudo apt install libglfw3-dev libgl1-mesa-dev\n"
        "then delete build/CMakeCache.txt and configure again.")
endif()

add_executable(brake_the_bricks
    src/main.cpp
    src/Ball.cpp
    src/Brick.cpp
    src/FallingObject.cpp
    src/Game.cpp
    src/Level.cpp
    src/Player.cpp
    src/Random.cpp
    src/Renderer.cpp
)
target_include_directories(brake_the_bricks PRIVATE src third_party)
target_link_libraries(brake_the_bricks PRIVATE OpenGL::GL glfw)
target_compile_options(brake_the_bricks PRIVATE -Wall -Wextra)
# Used when the executable runs from somewhere without an assets/ folder next to it.
target_compile_definitions(brake_the_bricks PRIVATE BTB_SOURCE_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")

# The game looks for assets/ next to the executable first.
add_custom_command(TARGET brake_the_bricks POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:brake_the_bricks>/assets)
