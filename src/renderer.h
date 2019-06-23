#ifndef SRC_RENDERER_H_
#define SRC_RENDERER_H_

#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

#include <string>
#include <vector>

#include "./face_collection.h"
#include "./shader.h"
#include "./shaping_cache.h"
#include "./state.h"
#include "./texture_atlas.h"

namespace renderer {
using face_collection::FaceCollection;
using shaping_cache::CodePointsFacePair;
using shaping_cache::ShapingCache;
using state::State;
using std::array;
using std::get;
using std::pair;
using std::string;
using std::vector;
using texture_atlas::Character;
using texture_atlas::TextureAtlas;
void Render(const Shader &shader, const vector<string> &lines,
            const FaceCollection &faces, ShapingCache *shaping_cache,
            const vector<TextureAtlas *> &texture_atlases, const State &state,
            GLuint VAO, GLuint VBO);
pair<Character, vector<unsigned char>> RenderGlyph(FT_Face face,
                                                   hb_codepoint_t codepoint);
} // namespace renderer

#endif // SRC_RENDERER_H_
