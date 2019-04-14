#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class Shader {
private:
  void checkGLShaderError(GLuint shaderId) {
    GLint status;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
      printf("shader didn't compile successfully");

      char buffer[512];
      glGetShaderInfoLog(shaderId, 512, nullptr, buffer);
      printf("%s\n", buffer);

      exit(EXIT_FAILURE);
    }
  }
  void checkGLProgramError(GLuint shaderProgramId_) {
    GLint status;
    glGetProgramiv(shaderProgramId_, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
      printf("program didn't link successfully");

      char buffer[512];
      glGetShaderInfoLog(shaderProgramId_, 512, nullptr, buffer);
      printf("%s\n", buffer);

      exit(EXIT_FAILURE);
    }
  }

public:
  // The program id
  GLuint shaderProgramId;

  Shader(const GLchar *vertexPath, const GLchar *fragmentPath) {
    std::string vertexCode, fragmentCode;
    std::ifstream vShaderFile, fShaderFile;
    // ensure ifstream objects can throw exceptions
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
      vShaderFile.open(vertexPath);
      fShaderFile.open(fragmentPath);
      std::stringstream vShaderStream, fShaderStream;

      vShaderStream << vShaderFile.rdbuf();
      fShaderStream << fShaderFile.rdbuf();

      vShaderFile.close();
      fShaderFile.close();

      vertexCode = vShaderStream.str();
      fragmentCode = fShaderStream.str();
    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
    }

    const char *vShaderCode = vertexCode.c_str();
    const char *fShaderCode = fragmentCode.c_str();

    GLuint vertexShaderId, fragmentShaderId;

    vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderId, 1, &vShaderCode, nullptr);
    glCompileShader(vertexShaderId);
    checkGLShaderError(vertexShaderId);

    fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderId, 1, &fShaderCode, nullptr);
    glCompileShader(fragmentShaderId);
    checkGLShaderError(fragmentShaderId);

    // Create a _program_ from this two shaders
    shaderProgramId = glCreateProgram();
    glAttachShader(shaderProgramId, vertexShaderId);
    glAttachShader(shaderProgramId, fragmentShaderId);
    glBindFragDataLocation(shaderProgramId, 0, "outColor");
    glLinkProgram(shaderProgramId);
    checkGLProgramError(shaderProgramId);

    // Delete the shader objects now that the program is linked
    glDeleteShader(vertexShaderId);
    glDeleteShader(fragmentShaderId);
  }

  // Use/activate the shader
  void use() { glUseProgram(shaderProgramId); }

  // Utility uniform functions
  void setBool(const std::string &name, bool value) const {
    glUniform1i(glGetUniformLocation(shaderProgramId, name.c_str()),
                static_cast<int>(value));
  }
  void setInt(const std::string &name, int value) const {
    glUniform1i(glGetUniformLocation(shaderProgramId, name.c_str()), value);
  }
  void setFloat(const std::string &name, float value) const {
    glUniform1f(glGetUniformLocation(shaderProgramId, name.c_str()), value);
  }
};

#endif
