// Copyright 2019 <Andrea Cognolato>
#include "./texture_atlas.h"

namespace texture_atlas {

TextureAtlas::TextureAtlas(GLsizei textureWidth, GLsizei textureHeight,
                           GLuint shaderProgramId,
                           const char* textureUniformLocation,
                           GLenum internalformat, GLenum format,
                           GLint shader_texture_index)
    : textureWidth_(textureWidth),
      textureHeight_(textureHeight),
      texture_cache_(kTextureDepth),
      fresh_(kTextureDepth),
      format_(format) {
  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texture_);
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, kMipLevelCount, internalformat,
                 textureWidth_, textureHeight_, kTextureDepth);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

  glUniform1i(glGetUniformLocation(shaderProgramId, textureUniformLocation),
              shader_texture_index);
}
TextureAtlas::~TextureAtlas() { glDeleteTextures(1, &texture_); }

void TextureAtlas::Insert(const vector<unsigned char>& bitmap_buffer,
                          GLsizei width, GLsizei height, Character* ch,
                          GLuint offset) {
  assert(static_cast<GLsizei>(offset) < kTextureDepth);

  GLint level = 0, xoffset = 0, yoffset = 0;
  GLsizei depth = 1;

  glBindTexture(GL_TEXTURE_2D_ARRAY, texture_);
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, xoffset, yoffset, offset, width,
                  height, depth, format_, GL_UNSIGNED_BYTE,
                  bitmap_buffer.data());
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

  auto v = glm::vec2(width / static_cast<GLfloat>(textureWidth_),
                     height / static_cast<GLfloat>(textureHeight_));
  ch->texture_id = texture_;
  ch->texture_array_index = offset;
  ch->texture_coordinates = v;
}

void TextureAtlas::Append(std::pair<Character, vector<unsigned char>>* p,
                          hb_codepoint_t codepoint) {
  Insert(p->second, p->first.size.x, p->first.size.y, &p->first, index_++);

  texture_cache_[codepoint] = p->first;
  fresh_[codepoint] = true;
}

void TextureAtlas::Replace(std::pair<Character, vector<unsigned char>>* p,
                           hb_codepoint_t stale, hb_codepoint_t codepoint) {
  Insert(p->second, p->first.size.x, p->first.size.y, &p->first,
         texture_cache_[stale].texture_array_index);

  texture_cache_.erase(stale);
  fresh_.erase(stale);

  texture_cache_[codepoint] = p->first;
  fresh_[codepoint] = true;
}

bool TextureAtlas::Contains(hb_codepoint_t codepoint) const {
  return texture_cache_.find(codepoint) != texture_cache_.end();
}

Character* TextureAtlas::Get(hb_codepoint_t codepoint) {
  auto it = texture_cache_.find(codepoint);
  if (it != texture_cache_.end()) {
    fresh_[codepoint] = true;
    return &it->second;
  }
  return nullptr;
}

void TextureAtlas::Insert(hb_codepoint_t codepoint,
                          pair<Character, vector<unsigned char>>& ch) {
  assert(texture_cache_.size() == fresh_.size());
  assert(!IsFull() || Contains_stale());

  if (!IsFull()) {
    Append(&ch, codepoint);
  } else {
    // Find the first stale one
    bool found = false;
    hb_codepoint_t stale;
    for (auto& kv : fresh_) {
      if (kv.second == false) {
        stale = kv.first;
        found = true;
      }
    }
    assert(found);
    Replace(&ch, stale, codepoint);
  }
}

bool TextureAtlas::IsFull() const {
  return (texture_cache_.size() == static_cast<size_t>(kTextureDepth));
}

bool TextureAtlas::Contains_stale() const {
  for (auto& kv : fresh_) {
    if (!kv.second) return true;
  }
  return false;
}

void TextureAtlas::Invalidate() {
  for (auto& kv : fresh_) {
    kv.second = false;
  }
}

GLuint TextureAtlas::GetTexture() const { return texture_; }

}  // namespace texture_atlas
