#include <map>

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

// Freetype
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H

// HarfBuzz
#include <harfbuzz/hb.h>
// HarfBuzz freetype
#include <harfbuzz/hb-ft.h>

void GLDebugMessageCallback(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length, const GLchar *msg,
                            const void *data) {
  char *_source;
  char *_type;
  char *_severity;

  switch (source) {
  case GL_DEBUG_SOURCE_API:
    _source = "API";
    break;

  case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
    _source = "WINDOW SYSTEM";
    break;

  case GL_DEBUG_SOURCE_SHADER_COMPILER:
    _source = "SHADER COMPILER";
    break;

  case GL_DEBUG_SOURCE_THIRD_PARTY:
    _source = "THIRD PARTY";
    break;

  case GL_DEBUG_SOURCE_APPLICATION:
    _source = "APPLICATION";
    break;

  case GL_DEBUG_SOURCE_OTHER:
    _source = "UNKNOWN";
    break;

  default:
    _source = "UNKNOWN";
    break;
  }

  switch (type) {
  case GL_DEBUG_TYPE_ERROR:
    _type = "ERROR";
    break;

  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
    _type = "DEPRECATED BEHAVIOR";
    break;

  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
    _type = "UDEFINED BEHAVIOR";
    break;

  case GL_DEBUG_TYPE_PORTABILITY:
    _type = "PORTABILITY";
    break;

  case GL_DEBUG_TYPE_PERFORMANCE:
    _type = "PERFORMANCE";
    break;

  case GL_DEBUG_TYPE_OTHER:
    _type = "OTHER";
    break;

  case GL_DEBUG_TYPE_MARKER:
    _type = "MARKER";
    break;

  default:
    _type = "UNKNOWN";
    break;
  }

  switch (severity) {
  case GL_DEBUG_SEVERITY_HIGH:
    _severity = "HIGH";
    break;

  case GL_DEBUG_SEVERITY_MEDIUM:
    _severity = "MEDIUM";
    break;

  case GL_DEBUG_SEVERITY_LOW:
    _severity = "LOW";
    break;

  case GL_DEBUG_SEVERITY_NOTIFICATION:
    _severity = "NOTIFICATION";
    break;

  default:
    _severity = "UNKNOWN";
    break;
  }

  printf("%d: %s of %s severity, raised from %s: %s\n", id, _type, _severity,
         _source, msg);
}

void processInput(GLFWwindow *window) {
  // Close window with ESC
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GL_TRUE);
  }
}

static const int WINDOW_WIDTH = 1000;
static const int WINDOW_HEIGHT = 300;
static const int FONT_ZOOM = 1;

// Comparing with 16px Visual Studio Code
static const int FONT_PIXEL_HEIGHT = 16;
static const int FONT_PIXEL_WIDTH = FONT_PIXEL_HEIGHT;

#define BACKGROUND_COLOR 35. / 255, 35. / 255, 35. / 255, 1.0f
#define FOREGROUND_COLOR 220. / 255, 218. / 255, 172. / 255, 1.0f

static const char *WINDOW_TITLE = "OpenGL";

GLuint VAO, VBO;

struct character {
  GLuint textureID;
  glm::ivec2 size;
  glm::ivec2 bearing;
  GLuint advance;
};

void renderCodepoint(FT_Face face,
                     std::map<hb_codepoint_t, character> &glyph_index_texures,
                     hb_codepoint_t glyph_index) {
  using std::cout;
  using std::endl;
  using std::pair;

  // Load the glyph
  if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT)) {
    cout << "Could not glyph index " << glyph_index << endl;
    exit(EXIT_FAILURE);
  }

  // Render the glyph (antialiased) into a RGB alpha map
  FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LCD);

  GLuint texture;
  {
    // Generate the texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    unsigned char *bitmap_buffer = (unsigned char *)malloc(
        face->glyph->bitmap.rows * face->glyph->bitmap.width);

    // face->glyph->bitmap.buffer is a rows * pitch matrix but we need a
    // matrix which is rows * width. For each row i, buffer[i][pitch] is just
    // a padding byte, therefore we can ignore it
    for (int i = 0; i < face->glyph->bitmap.rows; i++) {
      for (int j = 0; j < face->glyph->bitmap.width; j++) {
        unsigned char ch =
            face->glyph->bitmap.buffer[i * face->glyph->bitmap.pitch + j];
        bitmap_buffer[i * face->glyph->bitmap.width + j] = ch;
      }
    }

    GLsizei texure_width = face->glyph->bitmap.width / 3;

    // Load data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texure_width,
                 face->glyph->bitmap.rows, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 bitmap_buffer);

    free(bitmap_buffer);

    // Set texture wrapping options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Set linear filtering for minifying and magnifying
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    character ch = {.textureID = texture,
                    .size = glm::ivec2(texure_width, face->glyph->bitmap.rows),
                    .bearing = glm::ivec2(face->glyph->bitmap_left,
                                          face->glyph->bitmap_top),
                    .advance = static_cast<GLuint>(face->glyph->advance.x)};
    glyph_index_texures.insert(
        std::pair<hb_codepoint_t, character>(glyph_index, ch));
  }
}

void renderText(FT_Face face, Shader &s, std::string text, GLfloat x, GLfloat y,
                GLfloat scale,
                std::map<hb_codepoint_t, character> &glyph_index_texures) {
  // Create the harfbuzz buffer
  hb_buffer_t *buf = hb_buffer_create();

  // TODO: understand which shaping mode we are using
  // The default shaping model handles all non-complex scripts, and may also be
  // used as a fallback for handling unrecognized scripts.

  // Put the text in the buffer
  hb_buffer_add_utf8(buf, text.c_str(), -1, 0, -1);

  // Set the script, language and direction of the buffer
  hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
  hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
  hb_buffer_set_language(buf, hb_language_from_string("en", -1));
  hb_buffer_guess_segment_properties(buf);

  // Create a font using the face provided by freetype
  hb_font_t *font = hb_ft_font_create(face, NULL);

  hb_feature_t *features = (hb_feature_t *)calloc(sizeof(hb_feature_t), 3);
  assert(hb_feature_from_string("kern=1", -1, &features[0]));
  assert(hb_feature_from_string("liga=1", -1, &features[1]));
  assert(hb_feature_from_string("clig=1", -1, &features[2]));

  // Shape the font
  hb_shape(font, buf, &features[0], 3);

  unsigned int glyph_count = text.size();

  // Get the glyph and position information
  hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
  hb_glyph_position_t *glyph_pos =
      hb_buffer_get_glyph_positions(buf, &glyph_count);

  // Activate the texture unit
  glActiveTexture(GL_TEXTURE0);

  // Bind the vertex array object since we'll be using it in the loop
  glBindVertexArray(VAO);

  printf("Printing font info and rendering, complete string: %s\n",
         text.c_str());
  for (size_t i = 0; i < glyph_count; i++) {
    hb_codepoint_t codepoint = glyph_info[i].codepoint;
    hb_position_t x_offset = glyph_pos[i].x_offset >> 6;
    hb_position_t y_offset = glyph_pos[i].y_offset >> 6;
    hb_position_t x_advance = glyph_pos[i].x_advance >> 6;
    hb_position_t y_advance = glyph_pos[i].y_advance >> 6;

    printf("%c %4u %d %d %d %d %u\n", text[i], codepoint, x_offset, y_offset,
           x_advance, y_advance, FT_Get_Char_Index(face, text[i]));

    if (glyph_index_texures.find(codepoint) == glyph_index_texures.end()) {
      renderCodepoint(face, glyph_index_texures, codepoint);
    }
    character ch = glyph_index_texures.at(codepoint);

    GLfloat xpos = x + ch.bearing.x * scale;
    GLfloat ypos = y - (ch.size.y - ch.bearing.y) * scale;

    GLfloat w = ch.size.x * scale;
    GLfloat h = ch.size.y * scale;

    // Update VBO for each character
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

    // Bind the character's texure
    glBindTexture(GL_TEXTURE_2D, ch.textureID);

    // Update content of VBO memory
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Render quad
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Now advance cursors for next glyph (note that advance is number of 1/64
    // pixels)
    x += (ch.advance >> 6) * scale;
  }

  // Unbind VAO and texture
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  // glViewport(0, 0, width, height);
}

int main() {
  using std::cout;
  using std::endl;
  using std::pair;

  glfwInit(); // Init GLFW

  // Require OpenGL >= 4.0
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  // We want a context that only supports the new core functionality
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create a windowed window
  GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                        WINDOW_TITLE, nullptr, nullptr);

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // Make the current context active
  glfwMakeContextCurrent(window);

  // Check that glad worked
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    cout << "Failed to initialize OpenGL context" << endl;
    return -1;
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

  shader.use();
  glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(WINDOW_WIDTH),
                                    0.0f, static_cast<GLfloat>(WINDOW_HEIGHT));
  glUniformMatrix4fv(glGetUniformLocation(shader.programId, "projection"), 1,
                     GL_FALSE, glm::value_ptr(projection));

  // Initialize FreeType
  FT_Library ft;
  if (FT_Init_FreeType(&ft)) {
    cout << "Could not load freetype" << endl;
    exit(EXIT_FAILURE);
  }

  // Set which filter to use for the LCD Subpixel Antialiasing
  FT_Library_SetLcdFilter(ft, FT_LCD_FILTER_DEFAULT);

  {
    FT_Int maj, min, patch;
    FT_Library_Version(ft, &maj, &min, &patch);
    printf("FreeType version: %d.%d.%d\n", maj, min, patch);
  }

  // Load the face
  FT_Face face;
  if (FT_New_Face(ft, "./FiraCode-Retina.ttf", 0, &face)) {
    cout << "Could not load font" << endl;
    exit(EXIT_FAILURE);
  }

  // Request the size of the face.
  // TODO: this
  if (FT_Set_Pixel_Sizes(face, FONT_PIXEL_WIDTH, FONT_PIXEL_HEIGHT)) {
    cout << "Could not request the font size (in pixels)" << endl;
    exit(EXIT_FAILURE);
  }

  // Disable byte-alignment restriction
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  std::map<hb_codepoint_t, character> glyph_index_texures;

  // render the first [32,128] chars and save the textures in the
  // glyph_index_texures map
  for (GLubyte c = 32; c < 128; c++) {
    FT_Get_Char_Index(face, c);
    renderCodepoint(face, glyph_index_texures, FT_Get_Char_Index(face, c));
  }

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

  double limiterLastTime = glfwGetTime();

  double fpsLastTime = glfwGetTime();
  int frameCount = 0;

  // Closed event loop
  while (!glfwWindowShouldClose(window)) {
    double limiterCurrentTime = glfwGetTime();
    if (limiterCurrentTime > limiterLastTime + 1.f / 144.0) {
      // Update the fps counter and update the title if a second has passed
      frameCount++;
      double fpsCurrentTime = glfwGetTime();
      if (fpsCurrentTime - fpsLastTime >= 1.0) {
        char fps[128];
        sprintf(fps, "OpenGL - %d fps", frameCount);
        glfwSetWindowTitle(window, fps);
        frameCount = 0;
        fpsLastTime = fpsCurrentTime;
      }

      // Retrieve the window events
      glfwWaitEvents();
      processInput(window);

      // Set the background color
      glClearColor(BACKGROUND_COLOR);
      // Clear the colorbuffer
      glClear(GL_COLOR_BUFFER_BIT);

      // Draw
      // TODO: subpixel positioning (si puo' fare con freetype? lo fa gia?)
      // TODO: harfbuzz (shaping & kerning)
      // TODO: glyph cache/atlas
      // TODO: vedere schede ipad (skribo, atlas.swift)

      glm::vec4 fg_color(FOREGROUND_COLOR);
      glUniform4fv(glGetUniformLocation(shader.programId, "fg_color_sRGB"), 1,
                   glm::value_ptr(fg_color));

      renderText(face, shader, "<=< I HAS LIGATURES >=>", 0, 184, FONT_ZOOM,
                 glyph_index_texures);

      // Bind what we actually need to use
      glBindVertexArray(VAO);

      // Swap buffers when drawing is finished
      glfwSwapBuffers(window);
      limiterLastTime = limiterCurrentTime;
    }
  }

  // Free the face
  FT_Done_Face(face);
  // Free freetype library
  FT_Done_FreeType(ft);

  // Terminate GLFW
  glfwTerminate();
  return 0;
}
