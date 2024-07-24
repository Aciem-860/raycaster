#include "sprite.h"
#include <stdio.h>
#include <string.h>

static const sprite_t wooden_barrel_sprite = {"../wooden_barrel.png", 64, 64};
static const sprite_t iron_barrel_sprite = {"../iron_barrel.png", 64, 64};
static const sprite_t dinner_table_sprite = {"../dinner_table.png", 64, 64};
static const sprite_t well_water_sprite = {"../well.png", 64, 64};
static const sprite_t armor_sprite = {"../armor.png", 64, 64};
static const sprite_t furnace_sprite = {"../furnace.png", 64, 64};
static const sprite_t pillar_sprite = {"../pillar.png", 64, 64};
static const sprite_t empty_sprite = {"", 0, 0};
static sprite_type sprite_char(const char c);

prop_t props[MAP_HEIGHT * MAP_WIDTH];
int prop_number = 0;

void load_sprite_map(const char* path) {
    FILE* map_file;
    map_file = fopen(path, "r");
    char line[MAP_WIDTH + 1];
    int row = 0;
    while (fgets(line, MAP_WIDTH + 1, map_file) && row < MAP_HEIGHT) {
        if (strcmp(line, "\n")) {
            for (int col = 0; col < MAP_WIDTH; col++) {
                sprite_type _sp_type = sprite_char(line[col]);
                if (_sp_type != EMPTY) {
                    prop_t _prop = {_sp_type, {col * 64 + 32, row * 64 + 32}};
                    props[prop_number] = _prop;
                    prop_number++;
                }
            }
            row++;
        }
    }

    fclose(map_file);
}

static sprite_type sprite_char(const char c) {
    switch (c) {
    case 'w':
        return WOODEN_BARREL;
    case 'i':
        return IRON_BARREL;
    case 'd':
        return DINNER_TABLE;
    case 'f':
        return FURNACE;
    case 'e':
        return WELLWATER;
    case 'a':
        return ARMOR;
    case 'p':
        return PILLAR;
    default:
        return EMPTY;
    }
}

sprite_t get_sprite(sprite_type type) {
    switch (type) {
    case EMPTY:
        return empty_sprite;
    case WOODEN_BARREL:
        return wooden_barrel_sprite;
    case IRON_BARREL:
        return iron_barrel_sprite;
    case DINNER_TABLE:
        return dinner_table_sprite;
    case ARMOR:
        return armor_sprite;
    case FURNACE:
        return furnace_sprite;
    case WELLWATER:
        return well_water_sprite;
    case PILLAR:
        return pillar_sprite;
    }
    return empty_sprite;
}

int compare_props(const void* a, const void* b) {
    real_world_prop_t ra = *((real_world_prop_t*)a);
    real_world_prop_t rb = *((real_world_prop_t*)b);

    double da = ra.distance;
    double db = rb.distance;

    if (da == db)
        return 0;
    else if (da > db)
        return -1;
    else
        return 1;
}
