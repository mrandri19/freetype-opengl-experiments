// Copyright 2019 <Andrea Cognolato>
#ifndef SRC_UTIL_H_
#define SRC_UTIL_H_

#include <glad/glad.h>
#define UNUSED __attribute__((unused))

namespace util {

void GLDebugMessageCallback(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length UNUSED,
                            const GLchar *msg, const void *data UNUSED);

}  // namespace util
#endif  //  SRC_UTIL_H_
