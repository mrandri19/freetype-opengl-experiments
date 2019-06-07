// TODO(andrea): subpixel positioning (si puo' fare con freetype? lo fa gia?)
// TODO(andrea): statically link glfw, harfbuzz; making sure that both are
// TODO(andrea): benchmark startup time (500ms on emoji file, wtf)
// compiled in release mode

// 👚🔇🐕🏠 📗🍢💵📏🐁🌓 💼🐦👠
// 👚🔇🐕🏠 📗🍢💵📏🐁🌓 💼🐦👠

#include <chrono>
#include <ctime>
#include <fstream>
#include <ratio>
#include <unordered_map>
#include <vector>

// glad - OpenGL loader
#include <glad/glad.h>

// glm - OpenGL mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "texture_atlas.h"
#include "util.h"
#include "window.h"

// FreeType
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H

// HarfBuzz
#include <harfbuzz/hb.h>
// HarfBuzz FreeTpe
#include <harfbuzz/hb-ft.h>

namespace font_renderer {
using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

static const int kWindowWidth = 1024;
static const int kWindowHeight = 768;
static const int kFontPixelHeight = 17;
static const int kFontPixelWidth = kFontPixelHeight - 1;
static const float kFontZoom = 1;
static const int kLineHeight =
    static_cast<int>(kFontPixelHeight * 1.35); // Copied from VSCode's code
static const string kWindowTitle = "OpenGL";

#define BACKGROUND_COLOR 35. / 255, 35. / 255, 35. / 255, 1.0f
#define FOREGROUND_COLOR 220. / 255, 218. / 255, 172. / 255, 1.0f

const hb_codepoint_t CODEPOINT_MISSING_FACE = UINT32_MAX;
const hb_codepoint_t CODEPOINT_MISSING = UINT32_MAX;

GLuint VAO, VBO;

struct Character {
  glm::vec4 texture_coordinates;
  glm::ivec2 size;
  glm::ivec2 bearing;
  GLuint advance;
  bool colored;
};

struct State {
  size_t width;
  size_t height;

  size_t kLineHeight;

  size_t lines;

  ssize_t start_line; // must be signed to avoid underflow when subtracting 1
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
  if (yoffset > 0) { // going up
    if ((state.start_line - lines_to_scroll) >= 0) {
      state.start_line -= lines_to_scroll;
    }
  } else { // going down
    lines_to_scroll = -lines_to_scroll;
    if ((state.start_line + state.visible_lines + lines_to_scroll) <=
        state.lines) {
      state.start_line += lines_to_scroll;
    }
  }
}

// TODO(andrea): use a glyph atlas so that we don't have to switch textures for
// each codepoint rendered on the screen but instead just send an array of quads
void RenderCodepointToTexture(
    FT_Face face, unordered_map<hb_codepoint_t, Character> *codepoint_texures,
    hb_codepoint_t codepoint, TextureAtlas *atlas) {
  FT_Int32 flags = FT_LOAD_DEFAULT | FT_LOAD_TARGET_LCD;

  if (FT_HAS_COLOR(face)) {
    flags |= FT_LOAD_COLOR;
  }

  if (FT_Load_Glyph(face, codepoint, flags)) {
    fprintf(stderr, "Could not load glyph with codepoint: %u\n", codepoint);
    exit(EXIT_FAILURE);
  }

  // Render the glyph (antialiased) into a RGB alpha map
  // TODO(andrea): can this be removed when it has color?
  if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LCD)) {
    fprintf(stderr, "Could not render glyph with codepoint: %u\n", codepoint);
    exit(EXIT_FAILURE);
  }

  vector<unsigned char> bitmap_buffer(face->glyph->bitmap.rows *
                                      face->glyph->bitmap.width);
  if (!FT_HAS_COLOR(face)) {
    // face->glyph->bitmap.buffer is a rows * pitch matrix but we need a
    // matrix which is rows * width. For each row i, buffer[i][pitch] is
    // just a padding byte, therefore we can ignore it
    for (uint i = 0; i < face->glyph->bitmap.rows; i++) {
      for (uint j = 0; j < face->glyph->bitmap.width; j++) {
        unsigned char ch =
            face->glyph->bitmap.buffer[i * face->glyph->bitmap.pitch + j];
        bitmap_buffer[i * face->glyph->bitmap.width + j] = ch;
      }
    }
  }

  // If the glyph is not colored then it is subpixel antialiased so the
  // texture will have 3x the width
  GLsizei texture_width;
  GLsizei texture_height;

  if (FT_HAS_COLOR(face)) {
    texture_width = face->glyph->bitmap.width;
    texture_height = face->glyph->bitmap.rows;
  } else {
    texture_width = face->glyph->bitmap.width / 3;
    texture_height = face->glyph->bitmap.rows;
  }

  glm::vec4 texture_coordinates;

  if (FT_HAS_COLOR(face)) {
    texture_coordinates = atlas->AddColored(face->glyph->bitmap.buffer,
                                            texture_width, texture_height);
  } else {
    texture_coordinates = atlas->AddMonochromatic(
        bitmap_buffer.data(), texture_width, texture_height);
  }

  Character ch;
  ch.texture_coordinates = texture_coordinates;
  ch.size = glm::ivec2(texture_width, texture_height);
  ch.bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
  ch.advance = static_cast<GLuint>(face->glyph->advance.x);
  ch.colored = static_cast<bool> FT_HAS_COLOR(face);

  codepoint_texures->insert({codepoint, ch});
}

void RenderCodepointsToScreen(
    const vector<hb_codepoint_t> &codepoints, GLfloat x, GLfloat y,
    GLfloat scale, const Shader &shader,
    const unordered_map<hb_codepoint_t, Character> &glyph_index_texures,
    const TextureAtlas &texture_atlas) {
  // Bind the vertex array object since we'll be using it in the loop
  glBindVertexArray(VAO);

  for (size_t i = 0; i < codepoints.size(); i++) {
    auto codepoint = codepoints[i];

    if (glyph_index_texures.find(codepoint) == glyph_index_texures.end()) {
      assert(false);
    }

    Character ch = glyph_index_texures.at(codepoint);

    GLfloat xpos = x + ch.bearing.x * scale;
    GLfloat ypos = y - (ch.size.y - ch.bearing.y) * scale;
    GLuint advance = (ch.advance >> 6) * scale;

    GLfloat w = ch.size.x * scale, h = ch.size.y * scale;

    if (ch.colored) {
      GLfloat ratio_x = static_cast<GLfloat>(kFontPixelWidth) /
                        static_cast<GLfloat>(ch.size.x);
      GLfloat ratio_y = static_cast<GLfloat>(kFontPixelHeight) /
                        static_cast<GLfloat>(ch.size.y);

      w = ch.size.x * scale * ratio_x;
      h = ch.size.y * scale * ratio_y;

      xpos = x + ch.bearing.x * scale * ratio_x;
      ypos = y - (ch.size.y - ch.bearing.y) * scale * ratio_y;
      advance = w;
      glUniform1i(glGetUniformLocation(shader.programId, "colored"), 1);
    } else {
      glUniform1i(glGetUniformLocation(shader.programId, "colored"), 0);
    }

    auto tc = ch.texture_coordinates;

    // Update VBO for each Character
    // TODO(andrea): I guess this coordinates now are wrong?
    GLfloat vertices[6][4] = {/*
                           a
                           |
                           |
                           |
                           c--------b
                         */
                              {xpos, ypos + h, tc.x, tc.y},
                              {xpos, ypos, tc.x, tc.y + tc.w},
                              {xpos + w, ypos, tc.x + tc.z, tc.y + tc.w},
                              /*
                                                  d--------f
                                                           |
                                                           |
                                                           |
                                                           e
                                                */
                              {xpos, ypos + h, tc.x, tc.y},
                              {xpos + w, ypos, tc.x + tc.z, tc.y + tc.w},
                              {xpos + w, ypos + h, tc.x + tc.z, tc.y}};

    // Bind the texture to the active texture unit
    glActiveTexture(GL_TEXTURE0);

    // TODO(andrea): to draw both emojiis and text we need to switch between
    // two textures
    glBindTexture(GL_TEXTURE_2D, (ch.colored)
                                     ? texture_atlas.getColoredTexture()
                                     : texture_atlas.getMonochromaticTexture());
    {
      // Update content of VBO memory
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      { glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); }
      glBindBuffer(GL_ARRAY_BUFFER, 0);

      // Render quad
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    // Now advance cursors for next glyph (note that advance is number of
    // 1 / 64 pixels)
    x += advance;
  }

  // Unbind VAO
  glBindVertexArray(0);
}

vector<FT_Face> LoadFaces(FT_Library ft, const vector<string> &face_names) {
  vector<FT_Face> faces;

  for (auto face_name : face_names) {
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
    faces.push_back(face);
  }

  return faces;
}

void AssignCodepointsFaces(string text, vector<FT_Face> faces,
                           vector<size_t> *codepoint_faces,
                           vector<hb_codepoint_t> *codepoints,
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
    FT_Face face = faces[i];
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
      codepoints->resize(glyph_info_length, CODEPOINT_MISSING);
      codepoint_faces->resize(glyph_info_length, CODEPOINT_MISSING_FACE);
    }
    assert(glyph_info_length == codepoint_faces->size());

    // Asssign a face to each codepoint if the codepoint hasn't been assigned
    // yet
    for (size_t j = 0; j < glyph_info_length; j++) {
      hb_codepoint_t codepoint = glyph_info[j].codepoint;
      // hb_position_t x_offset = glyph_pos[j].x_offset >> 6;
      // hb_position_t y_offset = glyph_pos[j].y_offset >> 6;
      // hb_position_t x_advance = glyph_pos[j].x_advance >> 6;
      // hb_position_t y_advance = glyph_pos[j].y_advance >> 6;
      // TODO(andrea): use harfbuzz glyph info instead of FreeType's

      if (codepoint != 0 && (*codepoints)[j] == CODEPOINT_MISSING) {
        (*codepoints)[j] = codepoint;
        (*codepoint_faces)[j] = i;
      }

      // If we find a glyph which is not present in this face (therefore its
      // codepoint it's 0) and which has not been assigned already then we need
      // to iterate on the next font
      if (codepoint == 0 && (*codepoints)[j] == CODEPOINT_MISSING) {
        all_codepoints_have_a_face = true;
      }
    }

    // Free the face and the buffer
    hb_font_destroy(font);
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
  glDebugMessageCallback(GLDebugMessageCallback, nullptr);
}

void MainLoop(
    const Window &window, const Shader &shader, const vector<string> &lines,
    const vector<FT_Face> &faces,
    unordered_map<hb_codepoint_t, Character> *codepoint_texures,
    unordered_map<string, pair<vector<size_t>, vector<hb_codepoint_t>>>
        *shaping_cache,
    TextureAtlas *texture_atlas) {
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::high_resolution_clock;

  while (!glfwWindowShouldClose(window.window)) {
    // Set the background color
    glClearColor(BACKGROUND_COLOR);
    // Clear the colorbuffer
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw
    glm::vec4 fg_color(FOREGROUND_COLOR);
    glUniform4fv(glGetUniformLocation(shader.programId, "fg_color_sRGB"), 1,
                 glm::value_ptr(fg_color));

    {
      auto hits = 0;

      printf("Rendering %zu lines\n", state.visible_lines);
      auto t1 = high_resolution_clock::now();

      // Create the harfbuzz buffer
      hb_buffer_t *buf = hb_buffer_create();
      hb_buffer_pre_allocate(buf, lines[0].size());

      for (size_t i = state.start_line;
           i < (state.visible_lines + state.start_line); i++) {
        auto &line = lines[i];

        vector<size_t> codepoints_face_pair;
        vector<hb_codepoint_t> codepoints;

        auto it = shaping_cache->find(line);
        if (it == shaping_cache->end()) {
          AssignCodepointsFaces(line, faces, &codepoints_face_pair, &codepoints,
                                buf);
          for (size_t j = 0; j < codepoints_face_pair.size(); j++) {
            // If the codepoint has not been rendered yet
            if (codepoint_texures->find(codepoints[j]) ==
                codepoint_texures->end()) {
              // If we could not assign the character to any codepoint, fallback
              // to the last face
              if (codepoints_face_pair[j] == CODEPOINT_MISSING_FACE &&
                  codepoints[j] == CODEPOINT_MISSING) {
                const auto REPLACEMENT_CHARACTER = 0x0000FFFD;
                codepoints[j] =
                    FT_Get_Char_Index(faces[0], REPLACEMENT_CHARACTER);
                codepoints_face_pair[j] = 0;
              }

              assert(codepoints_face_pair[j] != CODEPOINT_MISSING_FACE);
              assert(codepoints[j] != CODEPOINT_MISSING);

              RenderCodepointToTexture(faces[codepoints_face_pair[j]],
                                       codepoint_texures, codepoints[j],
                                       texture_atlas);
            }
          }
          // XXX: does this get dereferenced?
          (*shaping_cache)[line] =
              std::make_pair(codepoints_face_pair, codepoints);
        } else {
          codepoints_face_pair = it->second.first;
          codepoints = it->second.second;
          hits++;
        }

        RenderCodepointsToScreen(codepoints, 0,
                                 kWindowHeight - (kLineHeight * kFontZoom *
                                                  ((i - state.start_line) + 1)),
                                 kFontZoom, shader, *codepoint_texures,
                                 *texture_atlas);
      }

      hb_buffer_destroy(buf);

      auto t2 = high_resolution_clock::now();
      auto time_span = duration_cast<duration<double>>(t2 - t1);
      printf("Rendering lines took %f ms (%3.0f fps/Hz),  hits rate: %3.2f\n",
             time_span.count() * 1000, 1.f / time_span.count(),
             static_cast<float>(hits) / state.visible_lines);
    }

    // Swap buffers when drawing is finished
    glfwSwapBuffers(window.window);

    glfwWaitEvents();
  }
}

int main(int argc UNUSED, char **argv) {
  Window window(kWindowWidth, kWindowHeight, kWindowTitle, KeyCallback,
                ScrollCallback);

  InitOpenGL();

  // Load the shaders
  Shader shader("src/shaders/text.v.glsl", "src/shaders/text.f.glsl");
  // Compile and link the shaders, then use them
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
    {
      glGenBuffers(1, &VBO);
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, nullptr,
                   GL_DYNAMIC_DRAW);

      GLuint location = glGetAttribLocation(shader.programId, "in_vertex");
      glEnableVertexAttribArray(location);
      glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE,
                            4 * sizeof(GLfloat), nullptr);

      glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
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
  }

  vector<string> face_names{"./FiraCode-Retina.ttf", "./NotoColorEmoji.ttf"};
  vector<FT_Face> faces = LoadFaces(ft, face_names);

  TextureAtlas texture_atlas;

  // TODO(andrea): handle changing viewport
  // Render only the lines visible in the viewport
  state.width = kWindowWidth;
  state.height = kWindowHeight;
  state.kLineHeight = kLineHeight;
  state.lines = lines.size();
  state.start_line = 0;
  state.visible_lines =
      ((static_cast<size_t>(floor(kWindowHeight / kLineHeight))) > lines.size())
          ? lines.size()
          : (static_cast<size_t>(ceil(kWindowHeight / kLineHeight)));

  unordered_map<hb_codepoint_t, Character> codepoint_texures;
  // TODO(andrea): invalidation and capacity logic (LRU?, Better Hashmap?)
  unordered_map<string, pair<vector<size_t>, vector<hb_codepoint_t>>>
      shaping_cache;
  shaping_cache.reserve(state.lines);

  MainLoop(window, shader, lines, faces, &codepoint_texures, &shaping_cache,
           &texture_atlas);

  for (auto face : faces) {
    FT_Done_Face(face);
  }

  FT_Done_FreeType(ft);

  return 0;
}

} // namespace font_renderer

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage %s FILE\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  font_renderer::main(argc, argv);

  return 0;
}
