cmake_minimum_required(VERSION 3.0.0)
project(opengl VERSION 0.1.0)

set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")


find_library(glfw3 REQUIRED)
find_library(freetype2 REQUIRED)
find_library(libpng16 REQUIRED)
find_library(harfbuzz REQUIRED)

include_directories(/usr/include/freetype2)
include_directories(/usr/include/libpng16)
include_directories(SYSTEM lib/glad/include)

add_executable(opengl
  src/main.cc
  src/texture_atlas.cc
  src/state.cc
  src/util.cc
  src/renderer.cc
  src/face_collection.cc
  lib/glad/src/glad.c
)

set_source_files_properties(lib/glad/src/glad.c PROPERTIES COMPILE_FLAGS -Wno-error -Wno-all -Wno-extra)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

target_compile_options(opengl PUBLIC -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic -g)
target_link_libraries(opengl glfw X11 dl freetype pthread harfbuzz)


# Try to find clang-tidy
find_program(
  CLANG_TIDY_EXE
  NAMES "clang-tidy"
  DOC "Path to clang-tidy executable"
  )
if(NOT CLANG_TIDY_EXE)
  message(STATUS "clang-tidy not found.")
else()
  message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
  set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}" "-checks=-*,google-*")
endif()

set_target_properties(
  opengl PROPERTIES
  CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
)

# If enabled, generates a compile_commands.json, used by clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS)
