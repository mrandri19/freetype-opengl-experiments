#ifndef LETTERA_TEXURE_ATLAS_H
#define LETTERA_TEXURE_ATLAS_H

#include <vector>

#include <glad/glad.h>
#include <harfbuzz/hb.h>
#include <glm/glm.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H

#include "util.h"

#include <cstdio>
#include <unordered_map>

namespace texture_atlas {
using std::vector;

static GLsizei kTextureDepth = 128;
static GLsizei kColoredMipLevelCount =
    2;  // TODO(andrea): change to 1, we arent using mipmap downscaling
static GLsizei kMonochromaticMipLevelCount = 1;
struct Character {
  size_t texture_array_index;
  glm::vec2 texture_coordinates;
  size_t texture_id;
  glm::ivec2 size;
  glm::ivec2 bearing;

  GLuint advance;

  bool colored;
};
class TextureAtlas {
 private:
  GLuint colored_offset_ = 0, monochromatic_offset_ = 0;
  GLuint colored_texture_, monochromatic_texture_;
  GLsizei coloredTextureWidth_, coloredTextureHeight_;
  GLsizei monochromaticTextureWidth_, monochromaticTextureHeight_;

  std::unordered_map<hb_codepoint_t, Character> texture_cache_;
  std::unordered_map<hb_codepoint_t, bool> fresh_;

  std::pair<Character, vector<unsigned char>> RenderGlyph(
      FT_Face face, hb_codepoint_t codepoint) {
    FT_Int32 flags = FT_LOAD_DEFAULT | FT_LOAD_TARGET_LCD;

    if (FT_HAS_COLOR(face)) {
      flags |= FT_LOAD_COLOR;
    }

    if (FT_Load_Glyph(face, codepoint, flags)) {
      fprintf(stderr, "Could not load glyph with codepoint: %u\n", codepoint);
      exit(EXIT_FAILURE);
    }

    if (!FT_HAS_COLOR(face)) {
      if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LCD)) {
        fprintf(stderr, "Could not render glyph with codepoint: %u\n",
                codepoint);
        exit(EXIT_FAILURE);
      }
    }

    vector<unsigned char> bitmap_buffer;
    if (!FT_HAS_COLOR(face)) {
      // face->glyph->bitmap.buffer is a rows * pitch matrix but we need a
      // matrix which is rows * width. For each row i, buffer[i][pitch] is
      // just a padding byte, therefore we can ignore it

      bitmap_buffer.resize(face->glyph->bitmap.rows *
                           face->glyph->bitmap.width * 3);
      for (uint i = 0; i < face->glyph->bitmap.rows; i++) {
        for (uint j = 0; j < face->glyph->bitmap.width; j++) {
          unsigned char ch =
              face->glyph->bitmap.buffer[i * face->glyph->bitmap.pitch + j];
          bitmap_buffer[i * face->glyph->bitmap.width + j] = ch;
        }
      }
    } else {
      bitmap_buffer.resize(face->glyph->bitmap.rows *
                           face->glyph->bitmap.width * 4);
      std::copy(face->glyph->bitmap.buffer,
                face->glyph->bitmap.buffer +
                    face->glyph->bitmap.rows * face->glyph->bitmap.width * 4,
                bitmap_buffer.begin());
    }

    GLsizei texture_width;
    GLsizei texture_height;
    if (FT_HAS_COLOR(face)) {
      texture_width = face->glyph->bitmap.width;
      texture_height = face->glyph->bitmap.rows;
    } else {
      // If the glyph is not colored then it is subpixel antialiased so the
      // texture will have 3x the width
      texture_width = face->glyph->bitmap.width / 3;
      texture_height = face->glyph->bitmap.rows;
    }

    Character ch;
    ch.size = glm::ivec2(texture_width, texture_height);
    ch.bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
    ch.advance = static_cast<GLuint>(face->glyph->advance.x);
    ch.colored = static_cast<bool> FT_HAS_COLOR(face);

    return std::make_pair(ch, bitmap_buffer);
  }

 public:
  TextureAtlas(GLsizei coloredTextureWidth, GLsizei coloredTextureHeight,
               GLsizei monochromaticTextureWidth,
               GLsizei monochromaticTextureHeight)
      : coloredTextureWidth_(coloredTextureWidth),
        coloredTextureHeight_(coloredTextureHeight),
        monochromaticTextureWidth_(monochromaticTextureWidth),
        monochromaticTextureHeight_(monochromaticTextureHeight),
        texture_cache_(kTextureDepth),
        fresh_(kTextureDepth) {
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

  // TODO(andrea): InsertColored and InsertMonochromatic can be DRYed up
  void InsertColored(vector<unsigned char> bitmap_buffer, GLsizei width,
                     GLsizei height, Character* ch, GLuint colored_offset) {
    printf("InsertColored offset:%d, w: %d, h: %d\n", colored_offset, width,
           height);

    GLint level = 0, xoffset = 0, yoffset = 0;
    GLsizei depth = 1;

    glBindTexture(GL_TEXTURE_2D_ARRAY, colored_texture_);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, xoffset, yoffset,
                    colored_offset, width, height, depth, GL_BGRA,
                    GL_UNSIGNED_BYTE, bitmap_buffer.data());
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    auto v = glm::vec2(width / (GLfloat)coloredTextureWidth_,
                       height / (GLfloat)coloredTextureHeight_);
    ch->texture_id = colored_texture_;
    ch->texture_array_index = colored_offset;
    ch->texture_coordinates = v;
    printf("InsertColored worked\n");
  };

  void InsertMonochromatic(vector<unsigned char> bitmap_buffer, GLsizei width,
                           GLsizei height, Character* ch,
                           GLuint monochromatic_offset) {
    assert(static_cast<GLsizei>(monochromatic_offset) < kTextureDepth);
    printf("InsertMonochromatic offset:%d, w: %d, h: %d\n",
           monochromatic_offset, width, height);

    GLint level = 0, xoffset = 0, yoffset = 0;

    glBindTexture(GL_TEXTURE_2D_ARRAY, monochromatic_texture_);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, xoffset, yoffset,
                    monochromatic_offset, width, height, 1, GL_RGB,
                    GL_UNSIGNED_BYTE, bitmap_buffer.data());
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    auto v = glm::vec2(width / (GLfloat)monochromaticTextureWidth_,
                       height / (GLfloat)monochromaticTextureHeight_);
    ch->texture_id = monochromatic_texture_;
    ch->texture_array_index = monochromatic_offset;
    ch->texture_coordinates = v;
  };

  Character Append(FT_Face face, hb_codepoint_t codepoint) {
    auto ch = RenderGlyph(face, codepoint);
    if (ch.first.colored) {
      InsertColored(ch.second, ch.first.size.x, ch.first.size.y, &ch.first,
                    colored_offset_++);
    } else {
      InsertMonochromatic(ch.second, ch.first.size.x, ch.first.size.y,
                          &ch.first, monochromatic_offset_++);
    }

    texture_cache_[codepoint] = ch.first;
    fresh_[codepoint] = true;

    return ch.first;
  };

  Character Replace(hb_codepoint_t old, FT_Face face,
                    hb_codepoint_t codepoint) {
    auto ch = RenderGlyph(face, codepoint);
    if (ch.first.colored) {
      InsertColored(ch.second, ch.first.size.x, ch.first.size.y, &ch.first,
                    texture_cache_[old].texture_array_index);
    } else {
      InsertMonochromatic(ch.second, ch.first.size.x, ch.first.size.y,
                          &ch.first, texture_cache_[old].texture_array_index);
    }

    texture_cache_.erase(old);
    fresh_.erase(old);
    texture_cache_[codepoint] = ch.first;
    fresh_[codepoint] = true;

    return ch.first;
  };

  bool IsFull() const {
    return (texture_cache_.size() == static_cast<size_t>(kTextureDepth));
  };
  bool Contains_stale() const {
    for (auto& kv : fresh_) {
      if (!kv.second) return true;
    }
    return false;
  };
  bool Contains(hb_codepoint_t codepoint) const {
    return texture_cache_.find(codepoint) != texture_cache_.end();
  };

  // TODO(andrea): sacrificare capretto
  Character GetOrInsert(FT_Face face, hb_codepoint_t codepoint) {
    assert(texture_cache_.size() == fresh_.size());

    assert(!IsFull() || Contains_stale());

    printf("GetOrInsert called, size: %zu\n", texture_cache_.size());

    auto it = texture_cache_.find(codepoint);
    if (it != texture_cache_.end()) {
      printf("codepoint found, returning it\n");
      fresh_[codepoint] = true;
      return it->second;
    } else {
      printf("codepoint not found\n");
      if (!IsFull()) {
        printf("there is space, appending\n");
        return Append(face, codepoint);
      } else {
        printf("there isnt't space but stale spots, replacing\n");
        // Find the first stale one
        bool found = false;
        hb_codepoint_t key;
        for (auto& kv : fresh_) {
          if (kv.second == 0) {
            key = kv.first;
            found = true;
          }
        }
        assert(found);
        return Replace(key, face, codepoint);
      }
    }
  };

  void Invalidate() {
    for (auto& kv : fresh_) {
      kv.second = false;
    }
  }

  GLuint GetColoredTexture() const { return colored_texture_; }
  GLuint GetMonochromaticTexture() const { return monochromatic_texture_; }
};
}  // namespace texture_atlas

#endif
