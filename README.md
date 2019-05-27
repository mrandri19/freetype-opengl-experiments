## Development

```shell
# Create the build directory
mkdir build
# Enter it
cd build
# Create the ninja build files using the ST2 Ninja template, feel free to use
# the regular build.ninja template
cmake -G "Sublime Text 2 - Ninja" ..
# Go back to the project root
cd ..
# Build the project and run it
ninja -C build && ./build/opengl
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
