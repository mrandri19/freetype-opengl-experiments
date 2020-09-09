## Development

```shell
git clone https://github.com/mrandri19/freetype-opengl-experiments

cd freetype-opengl-experiments

mkdir build

cd build

cmake -G Ninja .. 

cd ..

ninja -C build

./build/opengl README.md
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
