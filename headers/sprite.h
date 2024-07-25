#ifndef SPRITE_H
#define SPRITE_H

#include "constants.h"
#include "vector.h"
#include <stdbool.h>

typedef enum {
    EMPTY,
    WOODEN_BARREL,
    IRON_BARREL,
    DINNER_TABLE,
    FURNACE,
    ARMOR,
    WELLWATER,
    PILLAR,
    SOLDIER
} sprite_type;

typedef enum { PROP_IDLE = 0, PROP_DEAD } prop_state;

// TODO: Add a field to indicate if this sprite has collision
typedef struct {
    char* path;
    int width;
    int height;
    bool collision;
    int life_span; // -1 if no life
} sprite_t;

// NB: enemies are considered as props but they are not
// registered in the props array nevertheless
typedef struct {
    sprite_type type;
    vector_t position;
    prop_state state;
    int life;
} prop_t;

// Structure used to sort props to render by distance
typedef struct {
    prop_t prop;
    double distance;
} real_world_prop_t;

// ------------------------
// Global variables
// ------------------------

/// The list of enemies' indices in the props array
extern int enemy_index[100];
extern prop_t props[MAP_HEIGHT * MAP_WIDTH];
extern int prop_number;

extern const sprite_t wooden_barrel_sprite;
extern const sprite_t iron_barrel_sprite;
extern const sprite_t dinner_table_sprite;
extern const sprite_t well_water_sprite;
extern const sprite_t armor_sprite;
extern const sprite_t furnace_sprite;
extern const sprite_t pillar_sprite;
extern const sprite_t soldier_sprite;
extern const sprite_t empty_sprite;
// ------------------------
// Functions
// ------------------------

void load_sprite_map(const char* path);
sprite_t get_sprite(sprite_type type);
int compare_props(const void* a, const void* b);
prop_t* sprite_at_pos(int x, int y);
bool is_enemy(sprite_type t);

#endif
