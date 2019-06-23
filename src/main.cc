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

#include <fstream>
#include <unordered_map>
#include <vector>

// glm - OpenGL mathematics
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

#include "./callbacks.h"
#include "./constants.h"
#include "./renderer.h"
#include "./shader.h"
#include "./state.h"
#include "./texture_atlas.h"
#include "./util.h"
#include "./window.h"

namespace lettera {
using face_collection::FaceCollection;
using face_collection::LoadFaces;
using renderer::Render;
using shaping_cache::CodePointsFacePair;
using shaping_cache::ShapingCache;
using state::State;
using std::get;
using std::make_pair;
using std::make_tuple;
using std::pair;
using std::string;
using std::tuple;
using std::unordered_map;
using std::vector;
using texture_atlas::Character;
using texture_atlas::TextureAtlas;
using window::Window;

GLuint VAO, VBO;

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

int main(int argc UNUSED, char **argv) {
  Window window(kInitialWindowWidth, kInitialWindowHeight, kWindowTitle,
                callbacks::KeyCallback, callbacks::ScrollCallback,
                callbacks::ResizeCallback);
  State state(kInitialWindowWidth, kInitialWindowHeight, kLineHeight,
              kInitialLine);

  InitOpenGL();

  // Compile and link the shaders
  Shader shader("src/shaders/text.vert", "src/shaders/text.frag");
  shader.use();

  callbacks::glfw_user_pointer_t glfw_user_pointer;
  glfwSetWindowUserPointer(window.window, &glfw_user_pointer);

  glfw_user_pointer.shader_program_id = shader.programId;
  glfw_user_pointer.state = &state;

  // https://stackoverflow.com/questions/48491340/use-rgb-texture-as-alpha-values-subpixel-font-rendering-in-opengl
  // TODO(andrea): understand WHY it works, and if this is an actual solution,
  // then write a blog post
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);

  // Set the viewport
  glViewport(0, 0, kInitialWindowWidth, kInitialWindowHeight);

  // Disable byte-alignment restriction (our textures' size is not a multiple
  // of 4)
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  // Init Vertex Array Object (VAO)
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);
  // Init Vertex Buffer Object (VBO)
  glGenBuffers(1, &VBO);
  glBindVertexArray(0);

  // Init projection matrix
  glm::mat4 projection =
      glm::ortho(0.0f, static_cast<GLfloat>(kInitialWindowWidth), 0.0f,
                 static_cast<GLfloat>(kInitialWindowHeight));
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

  // Read the file
  vector<string> lines;
  {
    std::ifstream file(argv[1]);
    std::string line;
    while (std::getline(file, line)) {
      lines.push_back(line);
    }
    assert(lines.size() > 0);
  }
  glfw_user_pointer.lines = &lines;

  // Load the fonts
  // TODO(andrea): make this support multiple fonts
  vector<string> face_names{"./assets/fonts/FiraCode-Retina.ttf",
                            "./assets/fonts/NotoColorEmoji.ttf"};
  FaceCollection faces = LoadFaces(ft, face_names);
  // And the texture atlases
  TextureAtlas monochrome_texture_atlas(
      get<1>(faces[0]), get<2>(faces[0]), shader.programId,
      "monochromatic_texture_array", GL_RGB8, GL_RGB, 0);
  TextureAtlas colored_texture_atlas(get<1>(faces[1]), get<2>(faces[1]),
                                     shader.programId, "colored_texture_array",
                                     GL_RGBA8, GL_BGRA, 1);
  vector<TextureAtlas *> texture_atlases;
  texture_atlases.push_back(&monochrome_texture_atlas);
  texture_atlases.push_back(&colored_texture_atlas);

  // TODO(andrea): invalidation and capacity logic (LRU?, Better Hashmap?)
  // Init Shaping cache
  ShapingCache shaping_cache(state.GetVisibleLines());

  while (!glfwWindowShouldClose(window.window)) {
    glfwWaitEvents();

    auto t1 = glfwGetTime();

    Render(shader, lines, faces, &shaping_cache, texture_atlases, state, VAO,
           VBO);

    auto t2 = glfwGetTime();
    printf("Rendering lines took %f ms (%3.0f fps/Hz)\n", (t2 - t1) * 1000,
           1.f / (t2 - t1));

    // Swap buffers when drawing is finished
    glfwSwapBuffers(window.window);
  }

  for (auto &face : faces) {
    FT_Done_Face(get<0>(face));
  }

  FT_Done_FreeType(ft);

  return 0;
}
}  // namespace lettera

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage %s FILE\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  lettera::main(argc, argv);

  return 0;
}
