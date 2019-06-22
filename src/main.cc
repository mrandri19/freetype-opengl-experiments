// Copyright 2019 <Andrea Cognolato>
// TODO(andrea): subpixel positioning (si puo' fare con freetype? lo fa gia?)
// TODO(andrea): statically link glfw, harfbuzz; making sure that both are
// TODO(andrea): benchmark startup time (500ms on emoji file, wtf)
// compiled in release mode

// 👚🔇🐕🏠 📗🍢💵📏🐁🌓 💼🐦👠
// 👚🔇🐕🏠 📗🍢💵📏🐁🌓 💼🐦👠

// glad - OpenGL loader
#include <glad/glad.h>
// FreeType
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H

#include <GLFW/glfw3.h>

// HarfBuzz
#include <harfbuzz/hb.h>
// HarfBuzz FreeTpe
#include <harfbuzz/hb-ft.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <ratio>
#include <unordered_map>
#include <vector>

// glm - OpenGL mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "./shader.h"
#include "./texture_atlas.h"
#include "./util.h"
#include "./window.h"

namespace font_renderer {
using std::array;
using std::get;
using std::make_pair;
using std::make_tuple;
using std::min;
using std::pair;
using std::string;
using std::tuple;
using std::unordered_map;
using std::vector;
using texture_atlas::TextureAtlas;

static const int kWindowWidth = 1024;
static const int kWindowHeight = 768;
static const int kFontPixelHeight = 17;
static const int kFontPixelWidth = kFontPixelHeight - 1;
static const float kFontZoom = 1;
static const int kLineHeight =
    static_cast<int>(kFontPixelHeight * 1.35);  // Copied from VSCode's code
static const char kWindowTitle[] = "OpenGL";

// Dark+
#define FOREGROUND_COLOR 220. / 255, 218. / 255, 172. / 255, 1.0f
#define BACKGROUND_COLOR 35. / 255, 35. / 255, 35. / 255, 1.0f

const hb_codepoint_t CODEPOINT_MISSING_FACE = UINT32_MAX;
const hb_codepoint_t CODEPOINT_MISSING = UINT32_MAX;

GLuint VAO, VBO;

struct State {
  size_t width;
  size_t height;

  size_t kLineHeight;

  size_t lines;

  ssize_t start_line;  // must be signed to avoid underflow when subtracting 1
                       // to check if we can go up
  size_t visible_lines;
} state;

void KeyCallback(GLFWwindow *window, int key, int scancode UNUSED, int action,
                 int mods UNUSED) {
  if ((key == GLFW_KEY_DOWN || key == GLFW_KEY_J) &&
      (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    if ((state.start_line + state.visible_lines + 1) <= state.lines) {
      state.start_line++;
    }
  }
  if ((key == GLFW_KEY_UP || key == GLFW_KEY_K) &&
      (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    if ((state.start_line - 1) >= 0) {
      state.start_line--;
    }
  }
  if ((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
}

void ScrollCallback(GLFWwindow *window UNUSED, double xoffset UNUSED,
                    double yoffset) {
  int lines_to_scroll = yoffset;
  if (yoffset > 0) {  // going up
    if ((state.start_line - lines_to_scroll) >= 0) {
      state.start_line -= lines_to_scroll;
    }
  } else {  // going down
    lines_to_scroll = -lines_to_scroll;
    if ((state.start_line + state.visible_lines + lines_to_scroll) <=
        state.lines) {
      state.start_line += lines_to_scroll;
    }
  }
}

vector<tuple<FT_Face, GLsizei, GLsizei>> LoadFaces(
    FT_Library ft, const vector<string> &face_names) {
  vector<tuple<FT_Face, GLsizei, GLsizei>> faces;

  for (auto &face_name : face_names) {
    FT_Face face;
    if (FT_New_Face(ft, face_name.c_str(), 0, &face)) {
      fprintf(stderr, "Could not load font\n");
      exit(EXIT_FAILURE);
    }

    if (FT_HAS_COLOR(face)) {
      if (FT_Select_Size(face, 0)) {
        fprintf(stderr, "Could not request the font size (fixed)\n");
        exit(EXIT_FAILURE);
      }
    } else {
      if (FT_Set_Pixel_Sizes(face, kFontPixelWidth, kFontPixelHeight)) {
        fprintf(stderr, "Could not request the font size (in pixels)\n");
        exit(EXIT_FAILURE);
      }
    }

    // The face's size and bbox are populated only after set pixel sizes/select
    // size have been called
    GLsizei width, height;
    if (FT_IS_SCALABLE(face)) {
      width = FT_MulFix(face->bbox.xMax - face->bbox.xMin,
                        face->size->metrics.x_scale) >>
              6;
      height = FT_MulFix(face->bbox.yMax - face->bbox.yMin,
                         face->size->metrics.y_scale) >>
               6;
    } else {
      width = (face->available_sizes[0].width);
      height = (face->available_sizes[0].height);
    }
    faces.push_back(make_tuple(face, width, height));
  }

  return faces;
}

void AssignCodepointsFaces(
    string text, vector<tuple<FT_Face, GLsizei, GLsizei>> faces,
    pair<vector<size_t>, vector<hb_codepoint_t>> *codepoint_faces_pair,
    hb_buffer_t *buf) {
  // Flag to break the for loop when all of the codepoints have been assigned to
  // a face
  bool all_codepoints_have_a_face = true;
  for (size_t i = 0; (i < faces.size()) && all_codepoints_have_a_face; i++) {
    all_codepoints_have_a_face = false;

    // Reset the buffer, which is reused inbetween iterations
    hb_buffer_clear_contents(buf);

    // Put the text in the buffer
    hb_buffer_add_utf8(buf, text.data(), text.length(), 0, -1);

    // Set the script, language and direction of the buffer
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
    hb_buffer_set_language(buf, hb_language_from_string("en", -1));

    // Create a font using the face provided by freetype
    FT_Face face = get<0>(faces[i]);
    hb_font_t *font = hb_ft_font_create(face, nullptr);

    vector<hb_feature_t> features(3);
    assert(hb_feature_from_string("kern=1", -1, &features[0]));
    assert(hb_feature_from_string("liga=1", -1, &features[1]));
    assert(hb_feature_from_string("clig=1", -1, &features[2]));

    // Shape the font
    hb_shape(font, buf, &features[0], features.size());

    // Get the glyph and position information
    unsigned int glyph_info_length;
    // unsigned int glyph_position_length;
    hb_glyph_info_t *glyph_info =
        hb_buffer_get_glyph_infos(buf, &glyph_info_length);
    // hb_glyph_position_t *glyph_pos =
    // hb_buffer_get_glyph_positions(buf, &glyph_position_length);

    // assert(glyph_info_length == glyph_position_length);

    // Assign a size to the vector on the first iteration and fill it with
    // UINT32_MAX which represents the absence of a value This assumes that all
    // of the face runs will have the same lengths
    if (i == 0) {
      codepoint_faces_pair->first.resize(glyph_info_length,
                                         CODEPOINT_MISSING_FACE);
      codepoint_faces_pair->second.resize(glyph_info_length, CODEPOINT_MISSING);
    }
    assert(glyph_info_length == codepoint_faces_pair->first.size());

    // Asssign a face to each codepoint if the codepoint hasn't been assigned
    // yet
    for (size_t j = 0; j < glyph_info_length; j++) {
      hb_codepoint_t codepoint = glyph_info[j].codepoint;
      // hb_position_t x_offset = glyph_pos[j].x_offset >> 6;
      // hb_position_t y_offset = glyph_pos[j].y_offset >> 6;
      // hb_position_t x_advance = glyph_pos[j].x_advance >> 6;
      // hb_position_t y_advance = glyph_pos[j].y_advance >> 6;
      // TODO(andrea): use harfbuzz glyph info instead of FreeType's

      if (codepoint != 0 &&
          (codepoint_faces_pair->first)[j] == CODEPOINT_MISSING) {
        codepoint_faces_pair->first[j] = i;
        codepoint_faces_pair->second[j] = codepoint;
      }

      // If we find a glyph which is not present in this face (therefore its
      // codepoint it's 0) and which has not been assigned already then we need
      // to iterate on the next font
      if (codepoint == 0 &&
          (codepoint_faces_pair->second)[j] == CODEPOINT_MISSING) {
        all_codepoints_have_a_face = true;
      }
    }

    // Free the face and the buffer
    hb_font_destroy(font);
  }

  for (size_t i = 0; i < codepoint_faces_pair->first.size(); i++) {
    size_t face = codepoint_faces_pair->first[i];
    hb_codepoint_t codepoint = codepoint_faces_pair->second[i];

    if (face == CODEPOINT_MISSING_FACE && codepoint == CODEPOINT_MISSING) {
      const auto REPLACEMENT_CHARACTER = 0x0000FFFD;
      codepoint_faces_pair->first[i] = 0;
      codepoint_faces_pair->second[i] =
          FT_Get_Char_Index(get<0>(faces[0]), REPLACEMENT_CHARACTER);
    }
  }
}

void InitOpenGL() {
  // Check that glad worked
  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    fprintf(stderr, "glad failed to load OpenGL loader\n");
    exit(EXIT_FAILURE);
  }

  // Enable debug output
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(util::GLDebugMessageCallback, nullptr);
}

pair<texture_atlas::Character, vector<unsigned char>> RenderGlyph(
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

  texture_atlas::Character ch;
  ch.size = glm::ivec2(texture_width, texture_height);
  ch.bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
  ch.advance = static_cast<GLuint>(face->glyph->advance.x);
  ch.colored = static_cast<bool> FT_HAS_COLOR(face);

  return make_pair(ch, bitmap_buffer);
}

void RenderLine(
    const pair<vector<size_t>, vector<hb_codepoint_t>> &codepoints_face_pair,
    const vector<tuple<FT_Face, GLsizei, GLsizei>> &faces, GLfloat x, GLfloat y,
    GLfloat scale, const Shader &shader,
    const vector<TextureAtlas *> &texture_atlases) {
  glBindVertexArray(VAO);

  size_t codepoints_in_line = codepoints_face_pair.first.size();
  size_t i = 0;

  // While I haven't rendered all codepoints
  while (i < codepoints_in_line) {
    vector<texture_atlas::Character> characters;

    for (; i < codepoints_in_line; ++i) {
      hb_codepoint_t codepoint = codepoints_face_pair.second[i];

      texture_atlas::Character *ch = texture_atlases[0]->Get(codepoint);
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

    // TODO(andrea): instead of allocating on each line, allocate externally and
    // eventually resize here
    vector<array<array<GLfloat, 4>, 6>> quads;
    quads.resize(characters.size());
    vector<array<GLuint, 2>> texture_ids;
    texture_ids.resize(characters.size() * 6);

    for (size_t k = 0; k < characters.size(); ++k) {
      texture_atlas::Character &ch = characters[k];
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

        w = ch.size.x * scale * ratio_x;
        h = ch.size.y * scale * ratio_y;

        xpos = x + ch.bearing.x * scale * ratio_x;
        ypos = y - (ch.size.y - ch.bearing.y) * scale * ratio_y;
        advance = w;
      } else {
        w = ch.size.x * scale;
        h = ch.size.y * scale;

        xpos = x + ch.bearing.x * scale;
        ypos = y - (ch.size.y - ch.bearing.y) * scale;

        advance = (ch.advance >> 6) * scale;
      }
      x += advance;

      auto tc = ch.texture_coordinates;

      // FreeTypes uses a different coordinate convention so we need to render
      // the quad flipped horizontally, that's why where we should have 0 we
      // have tc.y and vice versa
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
      texture_ids[k * 6 + 0] = texture_ids[k * 6 + 1] = texture_ids[k * 6 + 2] =
          texture_ids[k * 6 + 3] = texture_ids[k * 6 + 4] =
              texture_ids[k * 6 + 5] = texture_id;
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
      glBufferSubData(GL_ARRAY_BUFFER, offset, quads_byte_size, quads.data());

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

      // Tell shader that layout=1 is an ivec2 starting after quads_byte_size
      glVertexAttribIPointer(1, 2, GL_UNSIGNED_INT, 2 * sizeof(GLuint),
                             reinterpret_cast<const GLvoid *>(offset));
      glEnableVertexAttribArray(1);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Render quads
    glDrawArrays(GL_TRIANGLES, 0, characters.size() * sizeof(characters[0]));

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    for (size_t j = 0; j < texture_atlases.size(); j++) {
      texture_atlases[j]->Invalidate();
    }
  }

  glBindVertexArray(0);
}

void Render(const Shader &shader, const vector<string> &lines,
            const vector<tuple<FT_Face, GLsizei, GLsizei>> &faces,
            unordered_map<string, pair<vector<size_t>, vector<hb_codepoint_t>>>
                *shaping_cache,
            const vector<TextureAtlas *> &texture_atlases) {
  // Set background color
  glClearColor(BACKGROUND_COLOR);
  glClear(GL_COLOR_BUFFER_BIT);

  // Create the shaping buffer
  hb_buffer_t *buf = hb_buffer_create();

  // For each visible line
  for (size_t i = state.start_line;
       i < (state.visible_lines + state.start_line); i++) {
    auto &line = lines[i];

    // Get from the cache which codepoints and faces to render the line with.
    // On miss, calculate and cache them
    auto it = shaping_cache->find(line);
    pair<vector<size_t>, vector<hb_codepoint_t>> codepoints_face_pair;
    if (it == shaping_cache->end()) {
      AssignCodepointsFaces(line, faces, &codepoints_face_pair, buf);
      (*shaping_cache)[line] = codepoints_face_pair;
    } else {
      codepoints_face_pair = it->second;
    }

    auto x = 0;
    auto y = kWindowHeight -
             (kLineHeight * kFontZoom * ((i - state.start_line) + 1));

    // Render the line to the screen, given the faces, the codepoints, the
    // codepoint's textures and a texture atlas
    RenderLine(codepoints_face_pair, faces, x, y, kFontZoom, shader,
               texture_atlases);
  }

  // Destroy the shaping buffer
  hb_buffer_destroy(buf);
}

int main(int argc UNUSED, char **argv) {
  Window window(kWindowWidth, kWindowHeight, kWindowTitle, KeyCallback,
                ScrollCallback);

  InitOpenGL();

  // Compile and link the shaders
  Shader shader("src/shaders/text.vert", "src/shaders/text.frag");
  shader.use();

  // https://stackoverflow.com/questions/48491340/use-rgb-texture-as-alpha-values-subpixel-font-rendering-in-opengl
  // TODO(andrea): understand WHY it works, and if this is an actual solution,
  // then write a blog post
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);

  // Set the viewport
  glViewport(0, 0, kWindowWidth, kWindowHeight);

  // Disable byte-alignment restriction (our textures' size is not a multiple of
  // 4)
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  // Init Vertex Array Object (VAO)
  {
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    // Init Vertex Buffer Object (VBO)
    glGenBuffers(1, &VBO);
    glBindVertexArray(0);
  }

  glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(kWindowWidth),
                                    0.0f, static_cast<GLfloat>(kWindowHeight));
  glUniformMatrix4fv(glGetUniformLocation(shader.programId, "projection"), 1,
                     GL_FALSE, glm::value_ptr(projection));

  // Initialize FreeType
  FT_Library ft;
  if (FT_Init_FreeType(&ft)) {
    fprintf(stderr, "Could not load freetype\n");
    exit(EXIT_FAILURE);
  }

  // Set which filter to use for the LCD Subpixel Antialiasing
  FT_Library_SetLcdFilter(ft, FT_LCD_FILTER_DEFAULT);

  vector<string> lines;
  {
    std::ifstream file(argv[1]);
    std::string line;
    while (std::getline(file, line)) {
      lines.push_back(line);
    }
    assert(lines.size() > 0);
  }

  vector<string> face_names{"./FiraCode-Retina.ttf", "./NotoColorEmoji.ttf"};
  // vector<string> face_names{"./UbuntuMono.ttf", "./NotoColorEmoji.ttf"};

  vector<tuple<FT_Face, GLsizei, GLsizei>> faces = LoadFaces(ft, face_names);

  // TODO(andrea): make this support multiple fonts
  TextureAtlas monochrome_texture_atlas(
      get<1>(faces[0]), get<2>(faces[0]), shader.programId,
      "monochromatic_texture_array", GL_RGB8, GL_RGB, 0);
  TextureAtlas colored_texture_atlas(get<1>(faces[1]), get<2>(faces[1]),
                                     shader.programId, "colored_texture_array",
                                     GL_RGBA8, GL_BGRA, 1);

  vector<TextureAtlas *> texture_atlases;
  texture_atlases.push_back(&monochrome_texture_atlas);
  texture_atlases.push_back(&colored_texture_atlas);

  // TODO(andrea): handle changing viewport
  // Render only the lines visible in the viewport
  state.width = kWindowWidth;
  state.height = kWindowHeight;
  state.kLineHeight = kLineHeight;
  state.lines = lines.size();
  state.start_line = 0;
  state.visible_lines =
      min(lines.size(), static_cast<size_t>(ceil(kWindowHeight / kLineHeight)));

  // TODO(andrea): invalidation and capacity logic (LRU?, Better Hashmap?)
  unordered_map<string, pair<vector<size_t>, vector<hb_codepoint_t>>>
      shaping_cache(state.lines);

  while (!glfwWindowShouldClose(window.window)) {
    auto t1 = std::chrono::high_resolution_clock::now();

    Render(shader, lines, faces, &shaping_cache, texture_atlases);

    auto t2 = std::chrono::high_resolution_clock::now();
    auto time_span =
        std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
    printf("Rendering lines took %f ms (%3.0f fps/Hz)\n",
           time_span.count() * 1000, 1.f / time_span.count());

    // Swap buffers when drawing is finished
    glfwSwapBuffers(window.window);

    glfwWaitEvents();
  }

  for (auto &face : faces) {
    FT_Done_Face(get<0>(face));
  }

  FT_Done_FreeType(ft);

  return 0;
}
}  // namespace font_renderer

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage %s FILE\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  font_renderer::main(argc, argv);

  return 0;
}
