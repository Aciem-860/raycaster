#ifndef GAME_H
#define GAME_H

#include "constants.h"
#include "sprite.h"
#include "vector.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdint.h>

// ========== GLOBAL VARIABLES ========== //

// ---------------
// World map
// ---------------

static char map[MAP_HEIGHT][MAP_WIDTH];
extern prop_t props[MAP_HEIGHT * MAP_WIDTH];

// ---------------
// Textures
// ---------------

static const char* textures_path = "../wolftextures.png";
static SDL_Texture* wall_texture;
static SDL_Texture* gun_texture;

// -------------------
// SDL Basic Colors
// -------------------

static const SDL_Color white = {0xff, 0xff, 0xff, 0xff};
static const SDL_Color gray = {0x80, 0x80, 0x80, 0xff};
static const SDL_Color black = {0x00, 0x00, 0x00, 0xff};
static const SDL_Color red = {0xff, 0x00, 0x00, 0xff};
static const SDL_Color green = {0x00, 0xff, 0x00, 0xff};
static const SDL_Color blue = {0x00, 0x00, 0x20, 0xff};
static const SDL_Color purple = {0x80, 0x00, 0x80, 0xff};
static const SDL_Color yellow = {0xff, 0xff, 0x00, 0xff};
static const SDL_Color orange = {0xff, 0xa5, 0x00, 0xff};

// -----------
// Player
// -----------

static const vector_t i_pos = {96, 64 * 10};
static const vector_t i_dir = {1, 0};
static player_t player = {i_pos, i_dir};

static bool is_firing;
static int ammo = 100;

// -------------------
// Mouse variables
// -------------------

static int cur_mouse_x, cur_mouse_y, prev_mouse_x, prev_mouse_y;

// -------------
// Framerate
// -------------

static uint32_t start_ticks, frame_ticks;

// --------------------------------
// Raycasting status variable
// --------------------------------

static int side = 0; // 0 = horizontal ; 1 = vertical
static int cx;
static int cy;
static bool hitx;
static bool hity;

// ========== FUNCTIONS ========== //

// ----------------------------
// Raycasting functions
// ----------------------------

vector_t find_next_point(vector_t pos, vector_t dir);
void load_map(const char* path);

// ---------------------
// Main Method
// ---------------------

/// Launch the game
int start();

#endif
