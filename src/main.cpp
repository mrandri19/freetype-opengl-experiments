#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "shader.hpp"

#ifdef UNUSED
#elif defined(__GNUC__)
#define UNUSED(x) UNUSED_##x __attribute__((unused))
#elif defined(__LCLINT__)
#define UNUSED(x) /*@unused@*/ x
#else
#define UNUSED(x) x
#endif

void debugMessageCallback(GLenum UNUSED(source), GLenum type, GLuint UNUSED(id),
                          GLenum severity, GLsizei UNUSED(length),
                          const GLchar *message,
                          const void *UNUSED(userParam)) {
  fprintf(stderr,
          "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
          (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity,
          message);
}

void processInput(GLFWwindow *window) {
  // Close window with ESC
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GL_TRUE);
  }
}

static const int WINDOW_WIDTH = 800;
static const int WINDOW_HEIGHT = 600;
static const char *WINDOW_TITLE = "OpenGL";

int main() {
  glfwInit(); // Init GLFW

  // Require OpenGL >= 3.2
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  // We want a context that only supports the new core functionality
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // Unresizable window

  // Create a windowed window
  GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                        WINDOW_TITLE, nullptr, nullptr);

  glfwMakeContextCurrent(window); // Make the current context active

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize OpenGL context" << std::endl;
    return -1;
  }
  printf("OpenGL %d.%d\n", GLVersion.major, GLVersion.minor);

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(debugMessageCallback, nullptr);

  // The data we will pass to the shaders
  float vertices[] = {
      // positions   // colors
      0.0f,  0.5f,  1.0f, 0.0f, 0.0f, // top right
      0.5f,  -0.5f, 0.0f, 1.0f, 0.0f, // bottom right
      -0.5f, -0.5f, 0.0f, 0.0f, 1.0f  // bottom left
  };
  unsigned int indices[] = {
      // note that we start from 0!
      0, 1, 2, // first triangle
  };

  // Element Buffer object
  GLuint EBO;
  glGenBuffers(1, &EBO);

  // Vertex Buffer Object
  GLuint VBO;
  glGenBuffers(1, &VBO);

  // Vertex Array Object, it is used to have "pointers" in the VBO
  GLuint VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  // Load the data into the buffers
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  // Initialize the shaders
  Shader shader("src/shaders/vertexShader.glsl",
                "src/shaders/fragmentShader.glsl");

  // Retrieve a reference to the vertex shader's "position" and "inColor"
  // variables
  GLint positionAttrib =
      glGetAttribLocation(shader.shaderProgramId, "position");
  GLint inColorAttrib = glGetAttribLocation(shader.shaderProgramId, "inColor");

  // Pass the first 2 values of each element of the vertices array to position
  glVertexAttribPointer(positionAttrib, 2, GL_FLOAT, GL_FALSE,
                        5 * sizeof(float), nullptr);
  glEnableVertexAttribArray(positionAttrib);

  // pass the next 3 to inColor
  glVertexAttribPointer(inColorAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(inColorAttrib);

  // Closed event loop
  while (!glfwWindowShouldClose(window)) {
    // Draw

    // Set background color
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shader.use();
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, sizeof(indices), GL_UNSIGNED_INT, nullptr);

    // Swap buffers when drawing is finished
    glfwSwapBuffers(window);

    // Retrieve the window events
    glfwPollEvents();

    processInput(window);
  }

  // Terminate GLFW
  glfwTerminate();
  return 0;
}
