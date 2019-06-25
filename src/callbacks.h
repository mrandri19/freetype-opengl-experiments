// Copyright 2019 <Andrea Cognolato>
#ifndef SRC_CALLBACKS_H_
#define SRC_CALLBACKS_H_

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <string>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

#include "./state.h"

#define UNUSED __attribute__((unused))

namespace callbacks {
using std::string;
using std::vector;
typedef struct {
  GLuint shader_program_id;
  const vector<string> *lines;
  state::State *state;
} glfw_user_pointer_t;

void KeyCallback(GLFWwindow *window, int key, int scancode UNUSED, int action,
                 int mods UNUSED) {
  auto obj =
      static_cast<glfw_user_pointer_t *>(glfwGetWindowUserPointer(window));
  auto state = obj->state;
  auto lines = obj->lines;
  if ((key == GLFW_KEY_DOWN || key == GLFW_KEY_J) &&
      (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    if ((state->GetStartLine() + state->GetVisibleLines() + 1) <=
        lines->size()) {
      state->GoUp(1);
    }
  }
  if ((key == GLFW_KEY_UP || key == GLFW_KEY_K) &&
      (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    // TODO(andrea): move this logic into the state instead
    if ((state->GetStartLine() - 1) >= 0) {
      state->GoDown(1);
    }
  }
  if ((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }

  if ((key == GLFW_KEY_HOME) && action == GLFW_PRESS) {
    state->GotoBeginning();
  }
  if ((key == GLFW_KEY_END) && action == GLFW_PRESS) {
    state->GotoEnd(lines->size());
  }
}

void ScrollCallback(GLFWwindow *window, double xoffset UNUSED, double yoffset) {
  int lines_to_scroll = yoffset;

  glfw_user_pointer_t *obj =
      static_cast<glfw_user_pointer_t *>(glfwGetWindowUserPointer(window));
  auto lines = obj->lines;

  auto state = obj->state;
  if (yoffset > 0) {  // going up
    if ((state->GetStartLine() - lines_to_scroll) >= 0) {
      state->GoDown(lines_to_scroll);
    }
  } else {  // going down
    lines_to_scroll = -lines_to_scroll;
    if ((state->GetStartLine() + state->GetVisibleLines() + lines_to_scroll) <=
        lines->size()) {
      state->GoUp(lines_to_scroll);
    }
  }
}

void ResizeCallback(GLFWwindow *window, int width, int height) {
  glfw_user_pointer_t *obj =
      static_cast<glfw_user_pointer_t *>(glfwGetWindowUserPointer(window));

  auto state = obj->state;
  glViewport(0, 0, width, height);
  glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(width), 0.0f,
                                    static_cast<GLfloat>(height));
  glUniformMatrix4fv(glGetUniformLocation(obj->shader_program_id, "projection"),
                     1, GL_FALSE, glm::value_ptr(projection));

  state->UpdateDimensions(width, height);
}

}  // namespace callbacks

#endif  // SRC_CALLBACKS_H_
