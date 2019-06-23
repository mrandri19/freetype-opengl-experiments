// Copyright 2019 <Andrea Cognolato>
#ifndef SRC_STATE_H_
#define SRC_STATE_H_

namespace state {
class State {
 public:
  State(unsigned int width, unsigned int height, unsigned int line_height,
        int start_line);
  ~State();
  void UpdateDimensions(unsigned int width, unsigned int height) {
    width_ = width;
    height_ = height;
    RecalculateVisibleLines();
  }
  int GetStartLine() const;
  unsigned int GetVisibleLines() const;
  unsigned int GetHeight() const;
  unsigned int GetLineHeight() const;
  void GoDown(unsigned int amount);
  void GoUp(unsigned int amount);
  void GotoBeginning();
  void GotoEnd(unsigned int lines_count);

 private:
  unsigned int width_;
  unsigned int height_;
  unsigned int line_height_;

  int start_line_;  // must be signed to avoid underflow when subtracting 1
                    // to check if we can go up
  unsigned int visible_lines_;

  void RecalculateVisibleLines();
};

}  // namespace state

#endif  // SRC_STATE_H_
