![Build status badge](https://github.com/mrandri19/freetype-opengl-experiments/workflows/CMake/badge.svg)

## Development

NOTE: These steps are copied from [the worflow file](https://github.com/mrandri19/freetype-opengl-experiments/blob/master/.github/workflows/cmake.yml)
which is the most up-to-date source on how to build the project.

```shell
git clone https://github.com/mrandri19/freetype-opengl-experiments
cd freetype-opengl-experiments

sudo apt-get update
sudo apt-get install libglfw3-dev libglm-dev

cmake -E make_directory build

cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug 
```

## Screenshots

The first text rendered with LCD Subpixel rendering.
![](docs/first_lcd_subpixel_rendering.png)

The first emojis rendered.
![](docs/first_working_emoji.png)

The first paragraph of emojis rendered.
![](docs/final_working_emoji_paragraph.png)

Emojis rendered with incorrect gamma blending.
![](docs/wrong_gamma_blending.png)

Emojis rendered with correct gamma blending.
![](docs/correct_gamma_blending.png)

Side by side comparison with VSCode
![](docs/side_by_side.png)

Side by side comparison with Sublime Text 3
![](docs/side_by_side_emoji.png)
