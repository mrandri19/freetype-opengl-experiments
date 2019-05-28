// TODO: subpixel positioning (si puo' fare con freetype? lo fa gia?)
// FIXME: crashes with too many emoji, wtf (use emojii ipsum to test it)

#include <chrono>
#include <ctime>
#include <fstream>
#include <ratio>
#include <unordered_map>
#include <vector>

// glad - OpenGL loader
#include <glad/glad.h>

// glfw - window and inputs
#include <GLFW/glfw3.h>

// glm - OpenGL mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Shader abstraction
#include "shader.hpp"

// Utils
#include "util.h"

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

struct state_t {
  size_t width;
  size_t height;

  size_t lines;

  size_t start_line;
  size_t visible_lines;
} state;

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    if ((state.start_line + state.visible_lines) < state.lines) {
      state.start_line++;
    }
  }
  if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    if (state.start_line > 0) {
      state.start_line--;
    }
  }
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
}

static const int WINDOW_WIDTH = 720;
static const int WINDOW_HEIGHT = 900;
static const int FONT_PIXEL_HEIGHT = 17;
static const int FONT_PIXEL_WIDTH = FONT_PIXEL_HEIGHT - 1;
static const int FONT_ZOOM = 1;
static const int LINE_HEIGHT = static_cast<int>(FONT_PIXEL_HEIGHT * 1.35);
static const char *WINDOW_TITLE = "OpenGL";

#define BACKGROUND_COLOR 35. / 255, 35. / 255, 35. / 255, 1.0f
#define FOREGROUND_COLOR 220. / 255, 218. / 255, 172. / 255, 1.0f
// #define FOREGROUND_COLOR 255. / 255, 0. / 255, 0. / 255, 1.0f

GLuint VAO, VBO;

struct character_t {
  GLuint textureID;
  glm::ivec2 size;
  glm::ivec2 bearing;
  GLuint advance;
  bool colored;
};

void render_codepoint_to_texture(
    FT_Face face, unordered_map<hb_codepoint_t, character_t> &codepoint_texures,
    hb_codepoint_t codepoint) {

  FT_Int32 flags = FT_LOAD_DEFAULT | FT_LOAD_TARGET_LCD;
  if (FT_HAS_COLOR(face)) {
    flags |= FT_LOAD_COLOR;
  }

  if (FT_Load_Glyph(face, codepoint, flags)) {
    fprintf(stderr, "Could not load glyph with codepoint: %d\n", codepoint);
    exit(EXIT_FAILURE);
  }

  // Render the glyph (antialiased) into a RGB alpha map
  if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LCD)) {
    fprintf(stderr, "Could not render glyph with codepoint: %d\n", codepoint);
    exit(EXIT_FAILURE);
  }

  {
    // TODO(performance): dont'use separate textures
    // Generate the texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    vector<unsigned char> bitmap_buffer(face->glyph->bitmap.rows *
                                        face->glyph->bitmap.width);

    if (!FT_HAS_COLOR(face)) {
      // face->glyph->bitmap.buffer is a rows * pitch matrix but we need a
      // matrix which is rows * width. For each row i, buffer[i][pitch] is
      // just a padding byte, therefore we can ignore it
      for (int i = 0; i < face->glyph->bitmap.rows; i++) {
        for (int j = 0; j < face->glyph->bitmap.width; j++) {
          unsigned char ch =
              face->glyph->bitmap.buffer[i * face->glyph->bitmap.pitch + j];
          bitmap_buffer[i * face->glyph->bitmap.width + j] = ch;
        }
      }
    }

    // 👚🔇🐕🏠 📗🍢💵📏🐁🌓 💼🐦👠

    // 👚🔇🐕🏠 📗🍢💵📏🐁🌓 💼🐦👠

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

    if (FT_HAS_COLOR(face)) {
      // Load data (the texture format is RGBA, 4 channes but the buffer
      // internal format is BGRA)
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width, texture_height, 0,
                   GL_BGRA, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

      // Generate mipmaps for the texture
      glGenerateMipmap(GL_TEXTURE_2D);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                      GL_LINEAR_MIPMAP_LINEAR);
    } else {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_width, texture_height, 0,
                   GL_RGB, GL_UNSIGNED_BYTE, &bitmap_buffer[0]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    // Set texture wrapping options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Set linear filtering for minifying and magnifying
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    character_t ch = {.textureID = texture,
                      .size = glm::ivec2(texture_width, texture_height),
                      .bearing = glm::ivec2(face->glyph->bitmap_left,
                                            face->glyph->bitmap_top),
                      .advance = static_cast<GLuint>(face->glyph->advance.x),
                      .colored = static_cast<bool> FT_HAS_COLOR(face)};
    codepoint_texures.insert({codepoint, ch});
  }
}

void render_codepoints_to_screen(
    const vector<hb_codepoint_t> &codepoints, GLfloat x, GLfloat y,
    GLfloat scale, Shader &shader,
    const unordered_map<hb_codepoint_t, character_t> &glyph_index_texures) {

  // Activate the texture unit
  glActiveTexture(GL_TEXTURE0);

  // Bind the vertex array object since we'll be using it in the loop
  glBindVertexArray(VAO);

  for (size_t i = 0; i < codepoints.size(); i++) {
    auto codepoint = codepoints[i];

    if (glyph_index_texures.find(codepoint) == glyph_index_texures.end()) {
      assert(false);
    }

    character_t ch = glyph_index_texures.at(codepoint);

    GLfloat xpos = x + ch.bearing.x * scale;
    GLfloat ypos = y - (ch.size.y - ch.bearing.y) * scale;
    GLuint advance = ch.advance >> 6;

    GLfloat w, h;

    if (ch.colored) {
      GLfloat ratio_x = (GLfloat)FONT_PIXEL_WIDTH / (GLfloat)ch.size.x;
      GLfloat ratio_y = (GLfloat)FONT_PIXEL_HEIGHT / (GLfloat)ch.size.y;

      w = ch.size.x * scale * ratio_x;
      h = ch.size.y * scale * ratio_y;

      xpos = x + ch.bearing.x * scale * ratio_x;
      ypos = y - (ch.size.y - ch.bearing.y) * scale * ratio_y;
      advance = w;
      glUniform1i(glGetUniformLocation(shader.programId, "colored"), 1);
    } else {
      w = ch.size.x * scale;
      h = ch.size.y * scale;
      glUniform1i(glGetUniformLocation(shader.programId, "colored"), 0);
    }

    // Update VBO for each character_t
    GLfloat vertices[6][4] = {/*
                                a
                                |
                                |
                                |
                                c--------b
                              */
                              {xpos, ypos + h, 0.0, 0.0},
                              {xpos, ypos, 0.0, 1.0},
                              {xpos + w, ypos, 1.0, 1.0},

                              /*
                                d--------f
                                         |
                                         |
                                         |
                                         e
                              */
                              {xpos, ypos + h, 0.0, 0.0},
                              {xpos + w, ypos, 1.0, 1.0},
                              {xpos + w, ypos + h, 1.0, 0.0}};

    // Bind the character_t's texure
    {
      glBindTexture(GL_TEXTURE_2D, ch.textureID);
      // Update content of VBO memory
      {
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
      }

      // Render quad
      glDrawArrays(GL_TRIANGLES, 0, 6);

      glBindTexture(GL_TEXTURE_2D, 0);
    }
    // Now advance cursors for next glyph (note that advance is number of
    // 1 / 64 pixels)
    // TODO: use harfbuzz glyph info instead of FreeType's
    x += (advance * scale);
  }

  // Unbind VAO and texture
  glBindVertexArray(0);
}

vector<FT_Face> load_faces(FT_Library ft, const vector<string> &face_names) {
  vector<FT_Face> faces;
  faces.reserve(face_names.size());

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
      if (FT_Set_Pixel_Sizes(face, FONT_PIXEL_WIDTH, FONT_PIXEL_HEIGHT)) {
        fprintf(stderr, "Could not request the font size (in pixels)\n");
        exit(EXIT_FAILURE);
      }
    }
    faces.push_back(face);
  }

  return faces;
}

void assign_codepoints_faces(string text, vector<FT_Face> faces,
                             vector<size_t> &codepoint_faces,
                             vector<hb_codepoint_t> &codepoints,
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
    unsigned int glyph_info_length, glyph_position_length;
    hb_glyph_info_t *glyph_info =
        hb_buffer_get_glyph_infos(buf, &glyph_info_length);
    hb_glyph_position_t *glyph_pos =
        hb_buffer_get_glyph_positions(buf, &glyph_position_length);

    assert(glyph_info_length == glyph_position_length);

    // Assign a size to the vector on the first iteration and fill it with -1
    // which represents the absence of a value
    // This assumes that all of the face runs will have the same lengths
    const auto MISSING = -1;
    if (i == 0) {
      codepoints.resize(glyph_info_length, MISSING);
      codepoint_faces.resize(glyph_info_length, MISSING);
    }
    assert(glyph_info_length == codepoint_faces.size());

    // Asssign a face to each codepoint if the codepoint hasn't been assigned
    // yet
    for (size_t j = 0; j < glyph_info_length; j++) {
      hb_codepoint_t codepoint = glyph_info[j].codepoint;
      hb_position_t x_offset = glyph_pos[j].x_offset >> 6;
      hb_position_t y_offset = glyph_pos[j].y_offset >> 6;
      hb_position_t x_advance = glyph_pos[j].x_advance >> 6;
      hb_position_t y_advance = glyph_pos[j].y_advance >> 6;

      if (codepoint != 0 && codepoints[j] == MISSING) {
        codepoints[j] = codepoint;
        codepoint_faces[j] = i;
      }

      // If we find a glyph which is not present in this face (therefore its
      // codepoint it's 0) and which has not been assigned already then we need
      // to iterate on the next font
      if (codepoint == 0 && codepoints[j] == MISSING) {
        all_codepoints_have_a_face = true;
      }
    }

    // Free the face and the buffer
    hb_font_destroy(font);
  }
}

int main(int argc, char **argv) {
  using namespace std::chrono;
  printf("Beginning setup\n");
  auto t1_ = high_resolution_clock::now();

  glfwInit(); // Init GLFW

  // Require OpenGL >= 4.0
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  // We want a context that only supports the new core functionality
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create a windowed window
  GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                        WINDOW_TITLE, nullptr, nullptr);

  // Make the current context active
  glfwMakeContextCurrent(window);

  // Check that glad worked
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    fprintf(stderr, "glad failed to load OpenGL loader\n");
    exit(EXIT_FAILURE);
  }

  // Enable debug output
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(GLDebugMessageCallback, nullptr);

  // https://stackoverflow.com/questions/48491340/use-rgb-texture-as-alpha-values-subpixel-font-rendering-in-opengl
  // TODO: understand WHY it works, and if this is an actual solution, then
  // write a blog post
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);

  // Set the viewport
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

  // Load the shaders
  Shader shader("src/shaders/text.v.glsl", "src/shaders/text.f.glsl");

  // Compile and link the shaders, then use them
  shader.use();
  glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(WINDOW_WIDTH),
                                    0.0f, static_cast<GLfloat>(WINDOW_HEIGHT));
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

  // Disable byte-alignment restriction
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  unordered_map<hb_codepoint_t, character_t> codepoint_texures;

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

  vector<string> lines;
  {
    std::ifstream file(argv[1]);
    std::string line;
    while (std::getline(file, line)) {
      lines.push_back(line);
    }
  }

  // vector<string> face_names{"./UbuntuMono.ttf", "./NotoColorEmoji.ttf"};
  vector<string> face_names{"./FiraCode-Retina.ttf", "./NotoColorEmoji.ttf"};
  vector<FT_Face> faces = load_faces(ft, face_names);

  auto t2_ = high_resolution_clock::now();
  auto time_span_ = duration_cast<duration<double>>(t2_ - t1_);
  printf("Setup took %f ms\n", time_span_.count() * 1000);

  // TODO: make actual calculations
  // Render only the lines visible in the viewport
  state = {
      .width = WINDOW_WIDTH,
      .height = WINDOW_HEIGHT,

      .lines = lines.size(),

      .start_line = 0,
      .visible_lines = (size_t)ceil(WINDOW_HEIGHT / LINE_HEIGHT),
  };

  glfwSetKeyCallback(window, key_callback);

  while (!glfwWindowShouldClose(window)) {

    // Set the background color
    glClearColor(BACKGROUND_COLOR);
    // Clear the colorbuffer
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw
    glm::vec4 fg_color(FOREGROUND_COLOR);
    glUniform4fv(glGetUniformLocation(shader.programId, "fg_color_sRGB"), 1,
                 glm::value_ptr(fg_color));

    {
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
        assign_codepoints_faces(line, faces, codepoints_face_pair, codepoints,
                                buf);

        for (size_t j = 0; j < codepoints_face_pair.size(); j++) {
          if (codepoint_texures.find(codepoints[j]) ==
              codepoint_texures.end()) {
            render_codepoint_to_texture(faces[codepoints_face_pair[j]],
                                        codepoint_texures, codepoints[j]);
          }
        }
        render_codepoints_to_screen(
            codepoints, 0,
            WINDOW_HEIGHT - (LINE_HEIGHT * ((i - state.start_line) + 1)),
            FONT_ZOOM, shader, codepoint_texures);
      }

      hb_buffer_destroy(buf);

      auto t2 = high_resolution_clock::now();
      auto time_span = duration_cast<duration<double>>(t2 - t1);
      printf("Rendering lines took %f ms (%3.0f fps/Hz)\n",
             time_span.count() * 1000, 1.f / time_span.count());
    }

    // Swap buffers when drawing is finished
    glfwSwapBuffers(window);

    glfwWaitEvents();
  }

  for (auto face : faces) {
    FT_Done_Face(face);
  }

  // Free freetype library
  FT_Done_FreeType(ft);

  // Terminate GLFW
  glfwTerminate();
  return 0;
} // namespace font_renderer

} // namespace font_renderer

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage %s FILE\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  font_renderer::main(argc, argv);

  return 0;
}
