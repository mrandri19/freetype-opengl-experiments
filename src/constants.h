// Copyright 2019 <Andrea Cognolato>
#ifndef SRC_CONSTANTS_H_
#define SRC_CONSTANTS_H_

static const unsigned int kInitialLine = 0;
static const unsigned int kInitialWindowWidth = 1024;
static const unsigned int kInitialWindowHeight = 768;
static const unsigned int kFontPixelHeight = 17;
static const unsigned int kFontPixelWidth = kFontPixelHeight - 1;
static const unsigned int kLineHeight =
    static_cast<int>(kFontPixelHeight * 1.35);  // Copied from VSCode's code
static const char kWindowTitle[] = "OpenGL";

// Dark+
#define FOREGROUND_COLOR 220. / 255, 218. / 255, 172. / 255, 1.0f
#define BACKGROUND_COLOR 35. / 255, 35. / 255, 35. / 255, 1.0f

#endif  // SRC_CONSTANTS_H_
