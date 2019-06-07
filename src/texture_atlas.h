#ifndef LETTERA_TEXURE_ATLAS_H
#define LETTERA_TEXURE_ATLAS_H

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "texture.h"
#include "util.h"

// TODO(andrea): remove
#include <cstdio>

// TODO(andrea): find an appropriate size for the texture and handle using
// multiple textures units
static GLsizei kTextureWidth = 1024;
static GLsizei kTextureHeight = 1024;

class TextureAtlas {
private:
  GLint colored_xoffset_ = 0, colored_yoffset_ = 0;
  GLint monochromatic_xoffset_ = 0, monochromatic_yoffset_ = 0;
  Texture colored_texture_, monochromatic_texture_;

public:
  TextureAtlas()
      : colored_texture_(kTextureWidth, kTextureWidth, GL_RGBA, GL_BGRA, true),
        monochromatic_texture_(kTextureWidth, kTextureWidth, GL_RGB, GL_RGB){};

  ~TextureAtlas(){};

  // TODO(andrea): AddColored and AddMonochromatic can be DRYed up
  glm::vec4 AddColored(unsigned char *bitmap_buffer, GLsizei width,
                       GLsizei height) {
    assert(colored_xoffset_ < kTextureWidth);
    assert(colored_yoffset_ < kTextureHeight);

    // If there is no space left go down
    if ((colored_xoffset_ + width) > kTextureWidth) {
      colored_xoffset_ = 0;
      colored_yoffset_ += (height * 2);
    }

    auto x = colored_xoffset_;
    auto y = colored_yoffset_;

    printf("AddColored x:%d, y:%d\n", x, y);

    colored_texture_.SubImage2D(x, y, width, height, GL_RGBA, bitmap_buffer);

    // Move right to occupy the next spot
    colored_xoffset_ += width;

    return glm::vec4(x / (GLfloat)kTextureWidth, y / (GLfloat)kTextureHeight,
                     width / (GLfloat)kTextureWidth,
                     height / (GLfloat)kTextureHeight);
  };

  glm::vec4 AddMonochromatic(unsigned char *bitmap_buffer, GLsizei width,
                             GLsizei height) {
    assert(monochromatic_xoffset_ < kTextureWidth);
    assert(monochromatic_yoffset_ < kTextureHeight);

    // If there is no space left go down
    if ((monochromatic_xoffset_ + width) > kTextureWidth) {
      monochromatic_xoffset_ = 0;
      monochromatic_yoffset_ += (height * 2);
    }
    auto x = monochromatic_xoffset_;
    auto y = monochromatic_yoffset_;

    // TODO(andrea): this texture doesn't render without mipmaps, why? in theory
    // the char
    monochromatic_texture_.SubImage2D(x, y, width, height, GL_RGB,
                                      bitmap_buffer);

    // Move right to occupy the next spot
    monochromatic_xoffset_ += width;

    return glm::vec4(x / (GLfloat)kTextureWidth, y / (GLfloat)kTextureHeight,
                     width / (GLfloat)kTextureWidth,
                     height / (GLfloat)kTextureHeight);
  };

  GLuint getColoredTexture() const { return colored_texture_.GetId(); }
  GLuint getMonochromaticTexture() const {
    return monochromatic_texture_.GetId();
  }
};

#endif
