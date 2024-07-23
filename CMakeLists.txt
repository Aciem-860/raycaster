cmake_minimum_required(VERSION 3.5)
project(raycasting)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 23)

FILE(GLOB SRCS sources/*.c)

include_directories(headers)
add_compile_options(-Wno-unused-variables -Wno-unused-const-variable -Wall -g -O3)
add_executable(raycasting ${SRCS})
target_link_libraries(raycasting SDL2 SDL2_image SDL2_ttf m)
