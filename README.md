![](https://raw.githubusercontent.com/mrandri19/freetype-opengl-experiments/master/Screenshot%20from%202019-05-19%2022-22-44.png)

```sh
sudo apt-get install build-essential libgl-dev libglfw3-dev libglew-dev
```

```sh
clang++ -Wall -Weverything -Wno-c++98-compat \
        -std=c++17 \
        -o main \
        -lGLEW -lglfw \
        main.cpp
```
