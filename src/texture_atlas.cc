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

  auto& item = texture_cache_[codepoint];
  item.character = p->first;
  item.fresh = true;
}

void TextureAtlas::Replace(std::pair<Character, vector<unsigned char>>* p,
                           hb_codepoint_t stale, hb_codepoint_t codepoint) {
  Insert(p->second, p->first.size.x, p->first.size.y, &p->first,
         texture_cache_[stale].character.texture_array_index);

  texture_cache_.erase(stale);

  auto& item = texture_cache_[codepoint];
  item.character = p->first;
  item.fresh = true;
}

bool TextureAtlas::Contains(hb_codepoint_t codepoint) const {
  return texture_cache_.find(codepoint) != texture_cache_.end();
}

Character* TextureAtlas::Get(hb_codepoint_t codepoint) {
  auto it = texture_cache_.find(codepoint);
  if (it != texture_cache_.end()) {
    it->second.fresh = true;
    return &(it->second.character);
  }
  return nullptr;
}

void TextureAtlas::Insert(hb_codepoint_t codepoint,
                          pair<Character, vector<unsigned char>>& ch) {
  assert(!IsFull() || Contains_stale());

  if (!IsFull()) {
    Append(&ch, codepoint);
  } else {
    // Find the first stale one
    bool found = false;
    hb_codepoint_t stale;
    for (auto& kv : texture_cache_) {
      if (kv.second.fresh == false) {
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
  for (auto& kv : texture_cache_) {
    if (!kv.second.fresh) return true;
  }
  return false;
}

void TextureAtlas::Invalidate() {
  for (auto& kv : texture_cache_) {
    kv.second.fresh = false;
  }
}

GLuint TextureAtlas::GetTexture() const { return texture_; }

}  // namespace texture_atlas
