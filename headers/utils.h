#ifndef UTILS_H
#define UTILS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

double absd(double v);
SDL_Color divide_by(SDL_Color c, int scalar);
bool set_color(SDL_Renderer* renderer, SDL_Color color);
bool set_window_color(SDL_Renderer* renderer, SDL_Color color);
Uint32 get_pixel(SDL_Surface* surface, int x, int y);
Uint32 correct_pixel(Uint32 pixel, SDL_Surface* surface);

#endif
