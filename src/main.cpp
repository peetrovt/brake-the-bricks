#include "Game.h"
#include "GameConstants.h"
#include "Renderer.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <system_error>

namespace {

struct WindowedPlacement {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

WindowedPlacement windowed;

// assets/ next to the executable (copied there by the build), else the one in the source tree.
std::string findAssetsDir() {
    std::error_code error;
    std::filesystem::path exe = std::filesystem::read_symlink("/proc/self/exe", error);
    if (!error) {
        std::filesystem::path besideExe = exe.parent_path() / "assets";
        if (std::filesystem::is_directory(besideExe, error)) {
            return besideExe.string();
        }
    }
    return BTB_SOURCE_ASSETS_DIR;
}

void toggleFullscreen(GLFWwindow* window) {
    if (glfwGetWindowMonitor(window)) {
        glfwSetWindowMonitor(window, nullptr, windowed.x, windowed.y, windowed.width, windowed.height, GLFW_DONT_CARE);
        return;
    }
    glfwGetWindowPos(window, &windowed.x, &windowed.y);
    glfwGetWindowSize(window, &windowed.width, &windowed.height);
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
}

void onKey(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (action != GLFW_PRESS) {
        return;
    }
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }
    if (key == GLFW_KEY_F11) {
        toggleFullscreen(window);
        return;
    }
    static_cast<btb::Game*>(glfwGetWindowUserPointer(window))->onKeyPress(key);
}

}  // namespace

int main() {
    glfwSetErrorCallback([](int code, const char* description) {
        std::cerr << "GLFW error " << code << ": " << description << '\n';
    });
    if (!glfwInit()) {
        return 1;
    }

    // 80% of the screen, 16:9 like the field; the picture is letterboxed if the window is resized.
    int width = 1280;
    int height = 720;
    if (const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor())) {
        width = std::min(mode->width * 4 / 5, mode->height * 4 / 5 * 16 / 9);
        height = width * 9 / 16;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    GLFWwindow* window = glfwCreateWindow(width, height, "Brake The Bricks 2", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    int exitCode = 0;
    try {
        btb::Game game(findAssetsDir());
        glfwSetWindowUserPointer(window, &game);
        glfwSetKeyCallback(window, onKey);

        double last = glfwGetTime();
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            double now = glfwGetTime();
            int direction = 0;
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
                --direction;
            }
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                ++direction;
            }
            game.update(now - last, direction);
            last = now;

            int framebufferWidth = 0;
            int framebufferHeight = 0;
            glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
            gfx::beginFrame(framebufferWidth, framebufferHeight);
            game.draw();
            glfwSwapBuffers(window);
        }
        glfwSetKeyCallback(window, nullptr);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        exitCode = 1;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return exitCode;
}
