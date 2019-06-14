cmake_minimum_required(VERSION 3.0.0)
project(opengl VERSION 0.1.0)

find_library(glfw3 REQUIRED)
find_library(freetype2 REQUIRED)
find_library(libpng16 REQUIRED)
find_library(harfbuzz REQUIRED)

include_directories(/usr/include/freetype2)
include_directories(/usr/include/libpng16)
include_directories(lib/glad/include)
include_directories(lib/doctest/include)

add_executable(opengl
  src/main.cc
  lib/glad/src/glad.c
)
add_executable(tests
  test/all.cc
)

target_compile_options(opengl PUBLIC -Werror -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic -g)
target_compile_options(tests PUBLIC -Werror -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic -g)
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

