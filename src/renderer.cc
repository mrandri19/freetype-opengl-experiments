// Copyright 2019 <Andrea Cognolato>
#include "./renderer.h"
#include "./constants.h"

namespace renderer {
void Render(const Shader &shader, const vector<string> &lines,
            const FaceCollection &faces, ShapingCache *shaping_cache,
            const vector<TextureAtlas *> &texture_atlases, const State &state,
            GLuint VAO, GLuint VBO) {
  // Set background color
  glClearColor(BACKGROUND_COLOR);
  glClear(GL_COLOR_BUFFER_BIT);

  // Create the shaping buffer
  hb_buffer_t *buf = hb_buffer_create();

  // Calculate how many lines to display
  unsigned int start_line = state.GetStartLine(), last_line;
  if (state.GetVisibleLines() > lines.size()) {
    last_line = start_line + lines.size();
  } else {
    last_line = start_line + state.GetVisibleLines();
  }

  // For each visible line
  for (unsigned int ix = start_line; ix < last_line; ix++) {
    auto &line = lines[ix];

    // Get from the cache which codepoints and faces to render the line with.
    // On miss, calculate and cache them
    auto it = shaping_cache->find(line);
    CodePointsFacePair codepoints_face_pair;
    if (it == shaping_cache->end()) {
      AssignCodepointsFaces(line, faces, &codepoints_face_pair, buf);
      (*shaping_cache)[line] = codepoints_face_pair;
    } else {
      codepoints_face_pair = it->second;
    }

    auto x = 0;
    auto y = state.GetHeight() -
             (state.GetLineHeight() * ((ix - state.GetStartLine()) + 1));

    // Render the line to the screen, given the faces, the codepoints, the
    // codepoint's textures and a texture atlas
    {
      glBindVertexArray(VAO);

      size_t codepoints_in_line = codepoints_face_pair.first.size();
      size_t i = 0;

      // While I haven't rendered all codepoints
      while (i < codepoints_in_line) {
        vector<Character> characters;

        for (; i < codepoints_in_line; ++i) {
          hb_codepoint_t codepoint = codepoints_face_pair.second[i];

          Character *ch = texture_atlases[0]->Get(codepoint);
          if (ch != nullptr) {
            characters.push_back(*ch);
            continue;
          }

          ch = texture_atlases[1]->Get(codepoint);
          if (ch != nullptr) {
            characters.push_back(*ch);
            continue;
          }

          // If I have got space in both
          if ((texture_atlases[0]->Contains_stale() ||
               !texture_atlases[0]->IsFull()) &&
              (texture_atlases[1]->Contains_stale() ||
               !texture_atlases[1]->IsFull())) {
            FT_Face face = get<0>(faces[codepoints_face_pair.first[i]]);

            // Get its texture's coordinates and offset from the atlas
            auto p = RenderGlyph(face, codepoint);
            if (p.first.colored) {
              texture_atlases[1]->Insert(codepoint, &p);
            } else {
              texture_atlases[0]->Insert(codepoint, &p);
            }
            characters.push_back(p.first);
          } else {
            break;
          }
        }

        // TODO(andrea): instead of allocating on each line, allocate externally
        // and eventually resize here
        vector<array<array<GLfloat, 4>, 6>> quads;
        quads.resize(characters.size());
        vector<array<GLuint, 2>> texture_ids;
        texture_ids.resize(characters.size() * 6);

        for (size_t k = 0; k < characters.size(); ++k) {
          Character &ch = characters[k];
          // Calculate the character position
          GLfloat w, h;
          GLfloat xpos, ypos;
          // TODO(andrea): we should use harfbuzz's info
          GLuint advance;
          if (ch.colored) {
            auto ratio_x = static_cast<GLfloat>(kFontPixelWidth) /
                           static_cast<GLfloat>(ch.size.x);
            auto ratio_y = static_cast<GLfloat>(kFontPixelHeight) /
                           static_cast<GLfloat>(ch.size.y);

            w = ch.size.x * ratio_x;
            h = ch.size.y * ratio_y;

            xpos = x + ch.bearing.x * ratio_x;
            ypos = y - (ch.size.y - ch.bearing.y) * ratio_y;
            advance = w;
          } else {
            w = ch.size.x;
            h = ch.size.y;

            xpos = x + ch.bearing.x;
            ypos = y - (ch.size.y - ch.bearing.y);

            advance = (ch.advance >> 6);
          }
          x += advance;

          auto tc = ch.texture_coordinates;

          // FreeTypes uses a different coordinate convention so we need to
          // render the quad flipped horizontally, that's why where we should
          // have 0 we have tc.y and vice versa
          array<array<GLfloat, 4>, 6> quad = {{// a
                                               // |
                                               // |
                                               // |
                                               // c--------b
                                               {xpos, ypos, 0, tc.y},
                                               {xpos, ypos + h, 0, 0},
                                               {xpos + w, ypos, tc.x, tc.y},
                                               // d--------f
                                               // |
                                               // |
                                               // |
                                               // e
                                               {xpos, ypos + h, 0, 0},
                                               {xpos + w, ypos, tc.x, tc.y},
                                               {xpos + w, ypos + h, tc.x, 0}}};
          array<GLuint, 2> texture_id = {
              static_cast<GLuint>(ch.texture_array_index),
              static_cast<GLuint>(ch.colored)};

          quads[k] = quad;
          texture_ids[k * 6 + 0] = texture_ids[k * 6 + 1] =
              texture_ids[k * 6 + 2] = texture_ids[k * 6 + 3] =
                  texture_ids[k * 6 + 4] = texture_ids[k * 6 + 5] = texture_id;
        }

        assert(6 * quads.size() == texture_ids.size());

        // Set the shader's uniforms
        glm::vec4 fg_color(FOREGROUND_COLOR);
        glUniform4fv(glGetUniformLocation(shader.programId, "fg_color_sRGB"), 1,
                     glm::value_ptr(fg_color));

        // Bind the texture to the active texture unit
        for (size_t j = 0; j < texture_atlases.size(); j++) {
          glActiveTexture(GL_TEXTURE0 + j);
          glBindTexture(GL_TEXTURE_2D_ARRAY, texture_atlases[j]->GetTexture());
        }

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        {
          // Allocate memory
          GLsizeiptr total_size =
              quads.size() * (sizeof(quads[0]) + 6 * sizeof(texture_ids[0]));
          glBufferData(GL_ARRAY_BUFFER, total_size, nullptr, GL_STREAM_DRAW);

          // Load quads
          GLintptr offset = 0;
          GLsizeiptr quads_byte_size = quads.size() * (sizeof(quads[0]));
          glBufferSubData(GL_ARRAY_BUFFER, offset, quads_byte_size,
                          quads.data());

          // Load texture_ids
          offset = quads_byte_size;
          GLsizeiptr texture_ids_byte_size =
              texture_ids.size() * (sizeof(texture_ids[0]));
          glBufferSubData(GL_ARRAY_BUFFER, offset, texture_ids_byte_size,
                          texture_ids.data());

          // Tell shader that layout=0 is a vec4 starting at offset 0
          glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                                nullptr);
          glEnableVertexAttribArray(0);

          // Tell shader that layout=1 is an ivec2 starting after
          // quads_byte_size
          glVertexAttribIPointer(1, 2, GL_UNSIGNED_INT, 2 * sizeof(GLuint),
                                 reinterpret_cast<const GLvoid *>(offset));
          glEnableVertexAttribArray(1);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quads
        glDrawArrays(GL_TRIANGLES, 0,
                     characters.size() * sizeof(characters[0]));

        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

        for (size_t j = 0; j < texture_atlases.size(); j++) {
          texture_atlases[j]->Invalidate();
        }
      }

      glBindVertexArray(0);
    }
  }

  // Destroy the shaping buffer
  hb_buffer_destroy(buf);
}

pair<Character, vector<unsigned char>> RenderGlyph(FT_Face face,
                                                   hb_codepoint_t codepoint) {
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
      fprintf(stderr, "Could not render glyph with codepoint: %u\n", codepoint);
      exit(EXIT_FAILURE);
    }
  }

  vector<unsigned char> bitmap_buffer;
  if (!FT_HAS_COLOR(face)) {
    // face->glyph->bitmap.buffer is a rows * pitch matrix but we need a
    // matrix which is rows * width. For each row i, buffer[i][pitch] is
    // just a padding byte, therefore we can ignore it

    bitmap_buffer.resize(face->glyph->bitmap.rows * face->glyph->bitmap.width *
                         3);
    for (uint i = 0; i < face->glyph->bitmap.rows; i++) {
      for (uint j = 0; j < face->glyph->bitmap.width; j++) {
        unsigned char ch =
            face->glyph->bitmap.buffer[i * face->glyph->bitmap.pitch + j];
        bitmap_buffer[i * face->glyph->bitmap.width + j] = ch;
      }
    }
  } else {
    bitmap_buffer.resize(face->glyph->bitmap.rows * face->glyph->bitmap.width *
                         4);
    copy(face->glyph->bitmap.buffer,
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

  return make_pair(ch, bitmap_buffer);
}

}  // namespace renderer
