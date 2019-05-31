#ifndef WINDOW_H
#define WINDOW_H

#include <GLFW/glfw3.h>
using std::string;

struct Window {
  GLFWwindow* window;

  Window(int width, int height, const string& title, GLFWkeyfun keyCallback,
         GLFWscrollfun scrollCallback) {
    glfwInit();  // Init GLFW

    // Require OpenGL >= 4.0
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // We want a context that only supports the new core functionality
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed window
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    glfwSetKeyCallback(window, keyCallback);
    glfwSetScrollCallback(window, scrollCallback);

    // Make the current context active
    glfwMakeContextCurrent(window);

    // Enable vsync
    glfwSwapInterval(1);
  }
  ~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  // Disable copy
  Window(const Window&) = delete;
  // Disable move
  Window& operator=(const Window&) = delete;
};

#endif  // WINDOW_H
