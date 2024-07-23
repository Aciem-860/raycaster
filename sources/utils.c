#include "utils.h"

double absd(double v) {
    int sgn = v > 0 ? 1 : -1;
    return sgn * v;
}

SDL_Color divide_by(SDL_Color c, int scalar) {
    SDL_Color _ret = {c.r / scalar, c.g / scalar, c.b / scalar, c.a};
    return _ret;
}

bool set_color(SDL_Renderer* renderer, SDL_Color color) {
    if (SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a) < 0) {
        return false;
    }
    return true;
}

bool set_window_color(SDL_Renderer* renderer, SDL_Color color) {
    if (!set_color(renderer, color)) {
        return false;
    }
    if (SDL_RenderClear(renderer) < 0) {
        return false;
    }
    return true;
}

Uint32 get_pixel(SDL_Surface* surface, int x, int y) {
    if (SDL_MUSTLOCK(surface)) {
        SDL_LockSurface(surface);
    }

    int bpp = surface->format->BytesPerPixel;
    Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
    Uint32 pixel;

    switch (bpp) {
    case 1:
        pixel = *p;
        break;
    case 2:
        pixel = *(Uint16*)p;
        break;
    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            pixel = p[0] << 16 | p[1] << 8 | p[2];
        } else {
            pixel = p[0] | p[1] << 8 | p[2] << 16;
        }
        break;
    case 4:
        pixel = *(Uint32*)p;
        break;
    default:
        pixel = 0; // Format not handled
        break;
    }

    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }
    return pixel;
}

Uint32 correct_pixel(Uint32 pixel, SDL_Surface* surface) {
    Uint8 r, g, b;
    SDL_GetRGB(pixel, surface->format, &r, &g, &b);
    Uint32 ret = b + (g << 8) + (r << 16);
    return ret;
}
