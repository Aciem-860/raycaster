# C Raycaster

## Short presentation

This project has the goal to create a Wolfenstein 3D like game. The project is written in C and uses SDL2 library.
It uses the raycasting method to create an impression of three-dimensional space.

## Dependencies

To run this project, you will need few additional libraries:
- SDL2
- SDL2_image
- SDL2_ttf

Under Archlinux, you can install it by doing

`pacman -S sdl2`

`pacman -S sdl2_image`

`pacman -S sdl2_ttf`

## Running the project

The project comes with a CMake file, then under Linux you can run

`mkdir -p build && cd build && cmake .. && make && ./raycasting`

# External Links

I encourage you to read the following [tutorial](https://lodev.org/cgtutor/raycasting.html) which is a great source of knowledge concerning raycasting methods. I use it to create my own raycaster engine.
