![](https://raw.githubusercontent.com/mrandri19/freetype-opengl-experiments/master/Screenshot%20from%202019-05-19%2022-22-44.png)

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

