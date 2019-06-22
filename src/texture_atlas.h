// Copyright 2019 <Andrea Cognolato>
// TODO(andrea): use a better map, try in some way to have dynamic gpu memory
// allocation, find a better data structure
#ifndef SRC_TEXTURE_ATLAS_H_
#define SRC_TEXTURE_ATLAS_H_

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H

#include <harfbuzz/hb.h>

#include <glad/glad.h>

#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

namespace texture_atlas {
using std::pair;
using std::unordered_map;
using std::vector;

static GLsizei kTextureDepth = 1024;
static GLsizei kMipLevelCount = 1;

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
  typedef struct {
    Character character;
    bool fresh;
  } cache_element_t;

  GLuint index_ = 0;
  GLuint texture_;
  GLsizei textureWidth_, textureHeight_;

  unordered_map<hb_codepoint_t, cache_element_t> texture_cache_;
  GLenum format_;

 public:
  TextureAtlas(GLsizei textureWidth, GLsizei textureHeight,
               GLuint shaderProgramId, const char* textureUniformLocation,
               GLenum internalformat, GLenum format,
               GLint shader_texture_index);

  ~TextureAtlas();

  void Insert(const vector<unsigned char>& bitmap_buffer, GLsizei width,
              GLsizei height, Character* ch, GLuint offset);

  void Append(pair<Character, vector<unsigned char>>* p,
              hb_codepoint_t codepoint);

  void Replace(pair<Character, vector<unsigned char>>* p, hb_codepoint_t stale,
               hb_codepoint_t codepoint);

  bool Contains(hb_codepoint_t codepoint) const;

  Character* Get(hb_codepoint_t codepoint);
  void Insert(hb_codepoint_t codepoint,
              pair<Character, vector<unsigned char>>* ch);

  pair<Character, vector<unsigned char>> RenderGlyph(FT_Face face,
                                                     hb_codepoint_t codepoint);

  bool IsFull() const;

  bool Contains_stale() const;

  void Invalidate();

  GLuint GetTexture() const;
};
}  // namespace texture_atlas

#endif  // SRC_TEXTURE_ATLAS_H_
