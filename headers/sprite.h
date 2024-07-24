#ifndef SPRITE_H
#define SPRITE_H

#include "constants.h"
#include "vector.h"

typedef enum { EMPTY, WOODEN_BARREL, IRON_BARREL, DINNER_TABLE } sprite_type;

// TODO: Add a field to indicate if this sprite has collision
typedef struct {
    char* path;
    int width;
    int height;
} sprite_t;

typedef struct {
    sprite_type type;
    vector_t position;
} prop_t;

// Structure used to sort props to render by distance
typedef struct {
    prop_t prop;
    double distance;
} real_world_prop_t;

extern prop_t props[MAP_HEIGHT * MAP_WIDTH];
extern int prop_number;
void load_sprite_map(const char* path);
sprite_t get_sprite(sprite_type type);
int compare_props(const void* a, const void* b);

#endif
