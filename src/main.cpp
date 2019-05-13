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

#include <ft2build.h>
#include FT_FREETYPE_H

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

static const int WINDOW_WIDTH = 640;
static const int WINDOW_HEIGHT = 480;
static const int FONT_ZOOM = 8;
// Comparing with 16px Visual Studio Code
static const int FONT_PIXEL_SIZE = 16 + 1;

static const char *WINDOW_TITLE = "OpenGL";

GLuint VAO, VBO;

struct Character {
  GLuint textureID;
  glm::ivec2 size;
  glm::ivec2 bearing;
  GLuint advance;
};
std::map<GLchar, Character> Characters;

void renderText(Shader &s, std::string text, GLfloat x, GLfloat y,
                GLfloat scale) {
  // Activate the texture unit
  glActiveTexture(GL_TEXTURE0);

  glBindVertexArray(VAO);

  for (const char c : text) {
    Character ch = Characters[c];

    GLfloat xpos = x + ch.bearing.x * scale;
    GLfloat ypos = y - (ch.size.y - ch.bearing.y) * scale;

    GLfloat w = ch.size.x * scale;
    GLfloat h = ch.size.y * scale;

    // Update VBO for each character
    GLfloat vertices[6][4] = {
        {xpos, ypos + h, 0.0, 0.0},    {xpos, ypos, 0.0, 1.0},
        {xpos + w, ypos, 1.0, 1.0},

        {xpos, ypos + h, 0.0, 0.0},    {xpos + w, ypos, 1.0, 1.0},
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
    x += (ch.advance / 64) * scale;
  }
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
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

  // Unresizable window
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  // Create a windowed window
  GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                        WINDOW_TITLE, nullptr, nullptr);

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

  // Load the face
  FT_Face face;
  if (FT_New_Face(ft, "UbuntuMono.ttf", 0, &face)) {
    cout << "Could not load font" << endl;
    exit(EXIT_FAILURE);
  }

  // Request the size of the face. 0, 16 means same as other
  if (FT_Set_Pixel_Sizes(face, 0, FONT_PIXEL_SIZE)) {
    cout << "Could not request the font size (in pixels)" << endl;
    exit(EXIT_FAILURE);
  }

  // Disable byte-alignment restriction
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  // render the first [32,128] chars and save the textures in the Characters map
  for (GLubyte c = 32; c < 128; c++) {
    // Get the character's glyph index
    FT_UInt glyph_index = FT_Get_Char_Index(face, c);

    // Load the glyph
    if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT)) {
      cout << "Could not load character " << c << endl;
      exit(EXIT_FAILURE);
    }

    // Render the glyph (antialiased) into a 256 grayscale pixmap
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

      // FIXME: WHY THE FUCK IS THIS SHIFTED BY 1PX?
      if (c == 'r') {
        // Print the bitmap buffer
        printf("character: %c\n", c);
        for (int i = 0; i < face->glyph->bitmap.rows; i++) {
          for (int j = 0; j < face->glyph->bitmap.width; j++) {
            unsigned char ch =
                face->glyph->bitmap.buffer[i * face->glyph->bitmap.pitch + j];
            ch != 0 ? printf("%3u ", ch) : printf("  . ");
          }
          printf("\n");
        }
        printf("\n");

        for (int i = 0; i < face->glyph->bitmap.rows; i++) {
          for (int j = 0; j < face->glyph->bitmap.width; j++) {
            unsigned char ch = bitmap_buffer[i * face->glyph->bitmap.width + j];
            ch != 0 ? printf("%3u ", ch) : printf("  . ");
          }
          printf("\n");
        }
        printf("\n");
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

      Character character = {
          .textureID = texture,
          .size = glm::ivec2(texure_width, face->glyph->bitmap.rows),
          .bearing =
              glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
          .advance = static_cast<GLuint>(face->glyph->advance.x)};
      Characters.insert(pair<GLchar, Character>(c, character));
    }
  }

  // Free the face
  FT_Done_Face(face);
  // Free freetype library
  FT_Done_FreeType(ft);

  // Enable blending (transparency)
  // glEnable(GL_BLEND);

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
      glfwPollEvents();
      processInput(window);

      // Clear the colorbuffer
      // glClearColor(29. / 255, 31. / 255, 33. / 255, 1.0f);
      glClearColor(1.0, 1.0, 1.0, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // Draw
      // TODO: subpixel antialiasing, I still dont understand the colors
      // TODO: kerning
      // TODO: harfbuzz
      // TODO: glyph cache/atlas
      // TODO: vedere schede ipad (skribo, atlas.swift)

      glm::vec4 fg_color(203. / 255, 206. / 255, 133. / 255, 1.0f);
      glUniform4fv(glGetUniformLocation(shader.programId, "fg_color"), 1,
                   glm::value_ptr(fg_color));

      glm::vec4 bg_color(35. / 255, 35. / 255, 35. / 255, 1.0f);
      glUniform4fv(glGetUniformLocation(shader.programId, "bg_color"), 1,
                   glm::value_ptr(bg_color));

      renderText(shader, "renderText", 0, 0, FONT_ZOOM);

      // Bind what we actually need to use
      glBindVertexArray(VAO);

      // Swap buffers when drawing is finished
      glfwSwapBuffers(window);
      limiterLastTime = limiterCurrentTime;
    }
  }

  // Terminate GLFW
  glfwTerminate();
  return 0;
}
