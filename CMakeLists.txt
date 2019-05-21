cmake_minimum_required(VERSION 3.0.0)
project(opengl VERSION 0.1.0)

find_library(glfw3 REQUIRED)
find_library(freetype2 REQUIRED)
find_library(libpng16 REQUIRED)
find_library(harfbuzz REQUIRED)

include_directories(/usr/include/freetype2)
include_directories(/usr/include/libpng16)
include_directories(lib/glad/include)

add_executable(opengl
  src/main.cpp
  lib/glad/src/glad.c
)

target_compile_options(opengl PUBLIC -Wno-writable-strings -Werror)
target_link_libraries(opengl glfw dl freetype harfbuzz)
