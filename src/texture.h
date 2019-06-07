#ifndef LETTERA_TEXTURE_H
#define LETTERA_TEXTURE_H

#include <glad/glad.h>

class Texture {
private:
  GLuint id_;
  bool withMipmaps_;

public:
  Texture(GLsizei width, GLsizei height, GLint internalformat, GLenum format,
          bool withMipmaps = false)
      : withMipmaps_(withMipmaps) {
    glGenTextures(1, &id_);

    // Set texture wrapping options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindTexture(GL_TEXTURE_2D, id_);
    {
      glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format,
                   GL_UNSIGNED_BYTE, nullptr);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

      if (withMipmaps_) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);
      } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
  };
  ~Texture() { glDeleteTextures(1, &id_); };

  void SubImage2D(GLint x, GLint y, GLsizei width, GLsizei height,
                  GLenum format, const GLvoid *pixels) {
    glBindTexture(GL_TEXTURE_2D, id_);
    {
      glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, format,
                      GL_UNSIGNED_BYTE, pixels);

      if (withMipmaps_) {
        glGenerateMipmap(GL_TEXTURE_2D);
      }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  GLuint GetId() const { return id_; }
};
#endif
