#ifndef LETTERA_UTIL_H
#define LETTERA_UTIL_H

#include <glad/glad.h>

#define UNUSED __attribute__((unused))

namespace util {

void GLDebugMessageCallback(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length UNUSED,
                            const GLchar *msg, const void *data UNUSED) {
  std::string _source, _type, _severity;

  switch (source) {
    case GL_DEBUG_SOURCE_API:
      _source = "API";
      break;

    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
      _source = "WINDOW SYSTEM";
      break;

    case GL_DEBUG_SOURCE_SHADER_COMPILER:
      _source = "SHADER COMPILER";
      break;

    case GL_DEBUG_SOURCE_THIRD_PARTY:
      _source = "THIRD PARTY";
      break;

    case GL_DEBUG_SOURCE_APPLICATION:
      _source = "APPLICATION";
      break;

    case GL_DEBUG_SOURCE_OTHER:
      _source = "UNKNOWN";
      break;

    default:
      _source = "UNKNOWN";
      break;
  }

  switch (type) {
    case GL_DEBUG_TYPE_ERROR:
      _type = "ERROR";
      break;

    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      _type = "DEPRECATED BEHAVIOR";
      break;

    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      _type = "UNDEFINED BEHAVIOR";
      break;

    case GL_DEBUG_TYPE_PORTABILITY:
      _type = "PORTABILITY";
      break;

    case GL_DEBUG_TYPE_PERFORMANCE:
      _type = "PERFORMANCE";
      break;

    case GL_DEBUG_TYPE_OTHER:
      _type = "OTHER";
      break;

    case GL_DEBUG_TYPE_MARKER:
      _type = "MARKER";
      break;

    default:
      _type = "UNKNOWN";
      break;
  }

  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
      _severity = "HIGH";
      break;

    case GL_DEBUG_SEVERITY_MEDIUM:
      _severity = "MEDIUM";
      break;

    case GL_DEBUG_SEVERITY_LOW:
      _severity = "LOW";
      break;

    case GL_DEBUG_SEVERITY_NOTIFICATION:
      _severity = "NOTIFICATION";
      break;

    default:
      _severity = "UNKNOWN";
      break;
  }

  // FIXME: ignore notifications
  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
    return;
  }

  printf("%d: %s of %s severity, raised from %s: %s\n", id, _type.c_str(),
         _severity.c_str(), _source.c_str(), msg);
  if (_severity == "HIGH" || _severity == "MEDIUM") assert(false);
}

}  // namespace util
#endif
