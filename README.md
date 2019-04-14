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
