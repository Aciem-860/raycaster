#ifndef CONSTANTS_H
#define CONSTANTS_H

#define TILE_WIDTH 64
#define TILE_HEIGHT 64
#define TEXTURE_WIDTH 64
#define TEXTURE_HEIGHT 64
#define MAP_WIDTH 21
#define MAP_HEIGHT 21
#define STEP_FORWARD 5
#define STEP_SIDE 5
#define DELTA_TIME 10
#define ANGLE_STEP DEG_TO_RAG(10)

#define WW 1280.0 // Window width
#define WH 640.0  // Window height
#define FOV 90.0  // Field of view in degrees
#define DEG_TO_RAG(x) (x * M_PI / 180)
#define FOVR DEG_TO_RAG(FOV)

#endif
