cmake_minimum_required(VERSION 3.0.0)
project(opengl VERSION 0.1.0)

find_library(glfw3 REQUIRED)

include_directories(lib/glad/include)
include_directories(src)

add_executable(opengl
  src/main.cpp
  lib/glad/src/glad.c
)

target_link_libraries(opengl glfw dl)
