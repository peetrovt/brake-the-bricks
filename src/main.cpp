#include "Game.h"
#include "GameConstants.h"
#include "Renderer.h"

#include <GLFW/glfw3.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <system_error>

namespace {

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

void onKey(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (action != GLFW_PRESS) {
        return;
    }
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    GLFWwindow* window = glfwCreateWindow(btb::kVisibleWidth, btb::kGameHeight, "Brake The Bricks 2", nullptr, nullptr);
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

            int width = 0;
            int height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            gfx::beginFrame(width, height);
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
