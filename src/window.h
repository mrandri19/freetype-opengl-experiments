// Copyright 2019 <Andrea Cognolato>

#ifndef SRC_WINDOW_H_
#define SRC_WINDOW_H_

#include <GLFW/glfw3.h>

#include <string>
using std::string;

struct Window {
  GLFWwindow* window;

  Window(int width, int height, const string& title, GLFWkeyfun keyCallback,
         GLFWscrollfun scrollCallback) {
    glfwInit();  // Init GLFW

    // Require OpenGL >= 4.6
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

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

#endif  // SRC_WINDOW_H_
