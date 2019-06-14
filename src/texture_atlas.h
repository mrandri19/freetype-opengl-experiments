#ifndef LETTERA_TEXURE_ATLAS_H
#define LETTERA_TEXURE_ATLAS_H

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "util.h"

// TODO(andrea): remove
#include <cstdio>

namespace texture_atlas {

// TODO(andrea): find an appropriate size for the texture and handle using
// multiple textures units
// TODO(andrea): LRU caching
static GLsizei kTextureDepth = 1024;
static GLsizei kColoredMipLevelCount = 2;
static GLsizei kMonochromaticMipLevelCount = 1;

class TextureAtlas {
 private:
  GLuint colored_offset_ = 0, monochromatic_offset_ = 0;
  GLuint colored_texture_, monochromatic_texture_;
  GLsizei coloredTextureWidth_, coloredTextureHeight_;
  GLsizei monochromaticTextureWidth_, monochromaticTextureHeight_;

 public:
  TextureAtlas(GLsizei coloredTextureWidth, GLsizei coloredTextureHeight,
               GLsizei monochromaticTextureWidth,
               GLsizei monochromaticTextureHeight)
      : coloredTextureWidth_(coloredTextureWidth),
        coloredTextureHeight_(coloredTextureHeight),
        monochromaticTextureWidth_(monochromaticTextureWidth),
        monochromaticTextureHeight_(monochromaticTextureHeight) {
    glGenTextures(1, &colored_texture_);
    glBindTexture(GL_TEXTURE_2D_ARRAY, colored_texture_);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, kColoredMipLevelCount, GL_RGBA8,
                   coloredTextureWidth_, coloredTextureHeight_, kTextureDepth);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    glGenTextures(1, &monochromatic_texture_);
    glBindTexture(GL_TEXTURE_2D_ARRAY, monochromatic_texture_);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, kMonochromaticMipLevelCount, GL_RGB8,
                   monochromaticTextureWidth, monochromaticTextureHeight,
                   kTextureDepth);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
  }

  ~TextureAtlas() {
    glDeleteTextures(1, &colored_texture_);
    glDeleteTextures(1, &monochromatic_texture_);
  };

  // TODO(andrea): AddColored and AddMonochromatic can be DRYed up
  std::tuple<GLuint, GLuint, glm::vec2> AddColored(unsigned char *bitmap_buffer,
                                                   GLsizei width,
                                                   GLsizei height) {
    assert(static_cast<GLsizei>(colored_offset_) < kTextureDepth);
    printf("AddColored offset:%d, w: %d, h: %d\n", colored_offset_, width,
           height);

    GLint level = 0, xoffset = 0, yoffset = 0;
    GLsizei depth = 1;

    glBindTexture(GL_TEXTURE_2D_ARRAY, colored_texture_);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, xoffset, yoffset,
                    colored_offset_, width, height, depth, GL_BGRA,
                    GL_UNSIGNED_BYTE, bitmap_buffer);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    colored_offset_++;

    auto v = glm::vec2(width / (GLfloat)coloredTextureWidth_,
                       height / (GLfloat)coloredTextureHeight_);
    return std::make_tuple(colored_texture_, colored_offset_ - 1, v);
  };

  std::tuple<GLuint, GLuint, glm::vec2> AddMonochromatic(
      unsigned char *bitmap_buffer, GLsizei width, GLsizei height) {
    assert(static_cast<GLsizei>(monochromatic_offset_) < kTextureDepth);
    printf("AddMonochromatic offset:%d, w: %d, h: %d\n", monochromatic_offset_,
           width, height);

    GLint level = 0, xoffset = 0, yoffset = 0;

    glBindTexture(GL_TEXTURE_2D_ARRAY, monochromatic_texture_);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, xoffset, yoffset,
                    monochromatic_offset_, width, height, 1, GL_RGB,
                    GL_UNSIGNED_BYTE, bitmap_buffer);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    monochromatic_offset_++;

    auto v = glm::vec2(width / (GLfloat)monochromaticTextureWidth_,
                       height / (GLfloat)monochromaticTextureHeight_);
    return std::make_tuple(monochromatic_texture_, monochromatic_offset_ - 1,
                           v);
  };
};
}  // namespace texture_atlas

#endif
