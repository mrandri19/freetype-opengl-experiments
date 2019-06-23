// Copyright 2019 <Andrea Cognolato>
#include "./state.h"

namespace state {
State::State(unsigned int width, unsigned int height, unsigned int line_height,
             int start_line)
    : width_(width),
      height_(height),
      line_height_(line_height),
      start_line_(start_line) {
  RecalculateVisibleLines();
}

State::~State() {}

void State::RecalculateVisibleLines() {
  visible_lines_ = height_ / line_height_;
}
int State::GetStartLine() const { return start_line_; }
unsigned int State::GetVisibleLines() const { return visible_lines_; }
unsigned int State::GetHeight() const { return height_; }
unsigned int State::GetLineHeight() const { return line_height_; }
void State::GoDown(unsigned int amount) { start_line_ -= amount; }
void State::GoUp(unsigned int amount) { start_line_ += amount; }
void State::GotoBeginning() { start_line_ = 0; }
void State::GotoEnd(unsigned int lines_count) {
  start_line_ = lines_count - visible_lines_;
}

}  // namespace state
