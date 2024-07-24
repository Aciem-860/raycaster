#include "game.h"
#include "sprite.h"
#include "utils.h"
#include "vector.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void load_map(const char* path) {
    FILE* map_file;
    map_file = fopen(path, "r");
    char line[MAP_WIDTH + 1];
    int row = 0;
    while (fgets(line, MAP_WIDTH + 1, map_file) && row < MAP_HEIGHT) {
        if (strcmp(line, "\n")) {
            for (int col = 0; col < MAP_WIDTH; col++) {
                map[row][col] = line[col];
            }
            row++;
        }
    }

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            printf("%c ", map[y][x]);
        }
        printf("\n");
    }

    fclose(map_file);
}

vector_t find_next_point(vector_t pos, vector_t dir) {
    double dydx = differential(dir);
    double _x, _y;

    vector_t dx;
    vector_t dy;

    cx = dir.x > 0 ? 1 : -1;
    cy = dir.y > 0 ? 1 : -1;

    // ----------
    // X axis
    // ----------

    if (dir.x > 0) { // Oriented to right
        _x = TILE_WIDTH - (int)(pos.x) % TILE_WIDTH;
    } else { // Oriented to left
        _x = -(int)(pos.x) % TILE_WIDTH;
        if (_x == 0)
            _x = -TILE_WIDTH;
    }
    vector_t _dx = {_x, _x * dydx};
    dx = _dx;

    // ----------
    // Y axis
    // ----------

    if (dir.y > 0) { // Oriented to down
        _y = TILE_HEIGHT - (int)(pos.y) % TILE_HEIGHT;
    } else { // Oriented to top
        _y = -(int)(pos.y) % TILE_HEIGHT;
        if (_y == 0)
            _y = -TILE_HEIGHT;
    }
    vector_t _dy = {_y / dydx, _y};
    dy = _dy;

    if (norm2(dy) < norm2(dx)) {
        hity = true;
        hitx = false;
        return add_vector(pos, dy);
    } else {
        hity = false;
        hitx = true;
        return add_vector(pos, dx);
    }
}

static bool hit_x() { return hitx; }
static bool hit_y() { return hity; }
static bool forward_x() { return cx == 1 ? true : false; }
static bool forward_y() { return cy == 1 ? true : false; }

int start() {

    // ---------------------
    // SDL Initializing
    // ---------------------

    SDL_Window* main_window = NULL;
    SDL_Renderer* renderer = NULL;

    SDL_Surface* texture_img = IMG_Load(textures_path);
    SDL_Surface* gun_surface = IMG_Load("../gun.png");

    int status = EXIT_FAILURE;

    if (0 != SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Error on SDL_Init: %s", SDL_GetError());
        goto Quit;
    }

    main_window = SDL_CreateWindow("Raycaster", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WW,
                                   WH, SDL_WINDOW_SHOWN);
    if (NULL == main_window) {
        fprintf(stderr, "Error on SDL_CreateWindow: %s", SDL_GetError());
        goto Quit;
    }

    renderer = SDL_CreateRenderer(main_window, -1, SDL_RENDERER_ACCELERATED);
    if (NULL == renderer) {
        fprintf(stderr, "Error on SDL_CreateRenderer: %s", SDL_GetError());
        goto Quit;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "Error on TTF_Init: %s", TTF_GetError());
        goto Quit;
    }

    TTF_Font* font;
    font = TTF_OpenFont("../Monocraft-nerd-fonts-patched.ttf", 24);
    if (!font) {
        fprintf(stderr, "Error at font loading: %s", TTF_GetError());
        goto Quit;
    }

    SDL_Event event;
    bool quit = false;

    // --------------------------------------------
    // Setting up texture and mouse handling
    // --------------------------------------------

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    wall_texture = SDL_CreateTextureFromSurface(renderer, texture_img);
    gun_texture = SDL_CreateTextureFromSurface(renderer, gun_surface);

    SDL_SetWindowGrab(main_window, SDL_TRUE);
    SDL_GetMouseState(&cur_mouse_x, &cur_mouse_y);
    prev_mouse_x = cur_mouse_x;
    prev_mouse_y = cur_mouse_y;

    // --------------------------
    // Framerate variables
    // --------------------------

    const int screen_fps = 60;
    const int screen_ticks_per_frame = 1000 / screen_fps;
    double fps = 0;

    // --------------------------
    // Raycasting parameters
    // --------------------------

    double angle = 0; // Angle made by dir vector with horizontal axis (left to right)
    double step_forward, step_side;
    vector_t cam_seg;

    // Loading the map
    load_map("../map");
    load_sprite_map("../sprite_map");

    // --------------------
    // Main game loop
    // --------------------

    while (!quit) {
        start_ticks = SDL_GetTicks();

        // Clear the screen
        set_window_color(renderer, blue);
        cam_seg = mult_vector(camera_segment(player), tan(FOVR / 2));

        // -----------------
        // Floor casting
        // -----------------

        Uint32 buffer[(int)WW * (int)WH];
        SDL_Texture* floor_texture =
            SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, WW, WH);
        for (int y = 0; y < WH / 2; y++) {
            double z = WH / 2;
            // Use Thales' Theorem and similar triangle
            double d = 64 * z / y; // d is the horizontal distance to the ground
            vector_t dir = mult_vector(player.dir, d);
            vector_t cam = mult_vector(cam_seg, d);
            vector_t lray = add_vector(player.pos, add_vector(dir, cam));
            vector_t rray = add_vector(player.pos, add_vector(dir, mult_vector(cam, -1)));

            double floor_step_x = (rray.x - lray.x) / WW;
            double floor_step_y = (rray.y - lray.y) / WW;

            vector_t floor = {lray.x, lray.y};

            for (int x = 0; x < WW; x++) {
                int tx_fl = 6 * TEXTURE_WIDTH + (int)floor.x % TEXTURE_WIDTH;
                int tx_cl = 4 * TEXTURE_WIDTH + (int)floor.x % TEXTURE_WIDTH;
                int ty = (int)floor.y % TEXTURE_HEIGHT;
                floor.x += floor_step_x;
                floor.y += floor_step_y;

                Uint32 _pf = get_pixel(texture_img, tx_fl, ty); // Floor
                Uint32 _pc = get_pixel(texture_img, tx_cl, ty); // Ceiling

                Uint8 r, g, b;
                SDL_GetRGB(_pf, texture_img->format, &r, &g, &b);

                Uint32 pixel_floor = correct_pixel(_pf, texture_img);
                Uint32 pixel_ceiling = correct_pixel(_pc, texture_img);
                buffer[x + (int)WW * (int)WH / 2 + (int)WW * y] = pixel_floor;
                buffer[x + (int)WW * (int)WH / 2 - (int)WW * y] = pixel_ceiling;
            }
        }
        SDL_SetRenderTarget(renderer, floor_texture);
        SDL_UpdateTexture(floor_texture, NULL, buffer, WW * sizeof(Uint32));
        SDL_Rect dst = {0, 0, WW, WH};
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, floor_texture, NULL, &dst);
        SDL_DestroyTexture(floor_texture);

        // ---------------
        // Wall casting
        // ---------------

        SDL_SetRenderTarget(renderer, wall_texture);
        double wall_distance[(int)WW];

        for (int x = 0; x < WW; x++) {
            double _frac = -((2.0 * x / WW) - 1);
            vector_t _cam_ray = mult_vector(cam_seg, _frac);
            vector_t _ray = add_vector((player.dir), _cam_ray);
            vector_t _hit = find_next_point(player.pos, _ray); // Intersection with grid

            vector_t p = player.pos;
            int _col, _row;
            for (int i = 0; i < 30; i++) {
                _hit = find_next_point(p, _ray);
                // set_color(top_renderer, yellow);

                _col = (int)_hit.x / TILE_WIDTH;
                _row = (int)_hit.y / TILE_HEIGHT;

                // Start by correction if a ray hits the top-left corner
                if ((int)_hit.x % TILE_WIDTH == 0 && (int)_hit.y % TILE_HEIGHT == 0 &&
                    map[_row][_col] == '.') {
                    if (cy == -1) {
                        _row--;
                    }
                    if (cx == -1) {
                        _col--;
                    }
                } else if (hit_x()) {
                    if (!forward_x()) {
                        _col--;
                    }
                } else if (hit_y()) {
                    if (!forward_y()) {
                        _row--;
                    }
                } else {
                    fprintf(stderr, "[ ERROR ] Should have hit something\n");
                    exit(EXIT_FAILURE);
                }

                if (map[_row][_col] != '.') {
                    break;
                }
                p = _hit;
            }

            // ----------------------
            // Shading the walls
            // ----------------------

            int _x = (int)_hit.x;
            int _y = (int)_hit.y;
            int _xmod = _x % TILE_WIDTH;
            int _ymod = _y % TILE_HEIGHT;

            if (_xmod == 0 && _ymod != 0) {
                side = 1;
            } else if (_xmod != 0 && _ymod == 0) {
                side = 0;
            }

            // ------------------------------------
            // Rendering the wall vertical stripe
            // ------------------------------------

            int _text_offset = 0;
            switch (map[_row][_col]) {
            case 'b': // Brick Wall
                _text_offset = 1 * TEXTURE_WIDTH;
                break;
            case 'f': // Brick Wall with flag
                _text_offset = 0;
                break;
            case 's': // Stone Wall
                _text_offset = 3 * TEXTURE_WIDTH;
                break;
            case 'g': // Blue Brick
                _text_offset = 4 * TEXTURE_WIDTH;
                break;
            case 'w': // Wooden wall
                _text_offset = 6 * TEXTURE_WIDTH;
                break;
            case 'm': // Mossy Stone Wall
                _text_offset = 5 * TEXTURE_WIDTH;
                break;
            case 't': // Terracota Wall
                _text_offset = 7 * TEXTURE_WIDTH;
                break;
            }

            double _distance = get_distance(player.pos, _hit);
            double _orthogonal_distance = get_cos(_ray, player.dir) * _distance;
            double _wall_height = 64 * WH / _orthogonal_distance;
            double _frac_text = side == 0 ? _xmod : _ymod;
            wall_distance[x] = _orthogonal_distance;

            SDL_Rect src = {_text_offset + _frac_text, 0, 1, TEXTURE_HEIGHT};
            SDL_Rect dst = {x, (WH - _wall_height) / 2, 1, _wall_height};
            SDL_RenderCopy(renderer, wall_texture, &src, &dst);
            if (!side) {
                SDL_Color _c = {0x0, 0x0, 0x0, 0x80};
                set_color(renderer, _c);
                SDL_RenderDrawLine(renderer, x, (WH - _wall_height) / 2, x,
                                   (WH + _wall_height) / 2);
            }
        }

        // ---------------------
        // Rendering props
        // ---------------------

        SDL_Surface* prop_surf;
        SDL_Texture* prop_text;

        real_world_prop_t props_to_render[prop_number];
        int _nb_props = 0; // Number of props to remder

        for (int i = 0; i < prop_number; i++) {
            // Ray from player to the prop
            vector_t ray = sub_vector(props[i].position, player.pos);
            double c = get_cos(ray, player.dir); // Cosine
            double phi = acos(c);
            double distance = norm2(ray);
            double orth_distance = distance * c;

            if (phi < FOVR / 2) {
                real_world_prop_t _prop = {props[i], orth_distance};
                props_to_render[_nb_props++] = _prop;
            }
        }

        // Sorting props by distance to the player
        if (_nb_props > 0) {
            qsort(props_to_render, _nb_props, sizeof(real_world_prop_t), compare_props);

            for (int p = 0; p < _nb_props; p++) {
                prop_t _prop = props_to_render[p].prop;
                // Ray from player to the prop
                vector_t ray = sub_vector(_prop.position, player.pos);
                double c = get_cos(ray, player.dir); // Cosine
                double s = get_cos(ray, cam_seg);    // Sine
                double distance = norm2(ray);
                double orth_distance = distance * c;                         // Orthogonal distance
                double x_offset = (WW / 2) * (distance * s) / orth_distance; // X-axis offset

                double w, h;
                w = 700 * 64 / orth_distance; // Number of column needed
                h = 700 * 64 / orth_distance;

                sprite_t sp = get_sprite(_prop.type);
                prop_surf = IMG_Load(sp.path);
                prop_text = SDL_CreateTextureFromSurface(renderer, prop_surf);
                SDL_SetRenderTarget(renderer, prop_text);

                for (int x = 0; x < w; x++) {
                    int _x = x * 64 / w;
                    if (orth_distance < wall_distance[(int)(WW / 2 - x_offset - w / 2 + x)]) {
                        SDL_Rect _src = {_x, 0, 1, TILE_HEIGHT};
                        SDL_Rect _dst = {WW / 2 - x_offset - w / 2 + x, WH / 2 - h / 2, 1, h};
                        SDL_RenderCopy(renderer, prop_text, &_src, &_dst);
                    }
                }
                SDL_DestroyTexture(prop_text);
                SDL_FreeSurface(prop_surf);
            }
        }

        // ---------------------
        // Rendering gun
        // ---------------------

        SDL_SetRenderTarget(renderer, gun_texture);
        const int gun_w = 500;
        const int gun_h = 500;
        SDL_Rect gun_dst = {(WW - gun_w) / 2, WH - gun_h, gun_w, gun_h};
        SDL_RenderCopy(renderer, gun_texture, NULL, &gun_dst);

        // ---------------------
        // Framerate printing
        // ---------------------

        SDL_Surface* text;
        char* framerate_txt;
        asprintf(&framerate_txt, "FPS: %d", (int)fps);
        text = TTF_RenderText_Solid(font, framerate_txt, yellow);
        SDL_Texture* text_texture;
        text_texture = SDL_CreateTextureFromSurface(renderer, text);
        SDL_Rect _text_pos = {0, 0, text->w, text->h};
        SDL_RenderCopy(renderer, text_texture, NULL, &_text_pos);

        // -----------------------------
        // Handling mouse for vision
        // -----------------------------

        angle = 0;
        SDL_GetMouseState(&cur_mouse_x, &cur_mouse_y);
        if (cur_mouse_x < 5) {
            SDL_WarpMouseInWindow(main_window, WW - 10, cur_mouse_y);
        } else if (cur_mouse_x > WW - 5) {
            SDL_WarpMouseInWindow(main_window, 10, cur_mouse_y);
        }

        int mouse_delta = prev_mouse_x - cur_mouse_x;
        if (abs(mouse_delta) < 250) {
            angle += (double)mouse_delta / 500;
        }

        SDL_RenderPresent(renderer);

        // -----------------------------
        // Handling keyboard events
        // -----------------------------

        step_forward = 0;
        step_side = 0;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_x:
                    quit = true;
                    break;
                case SDLK_z:
                    step_forward += STEP_FORWARD;
                    break;
                case SDLK_s:
                    step_forward -= STEP_FORWARD;
                    break;
                case SDLK_q:
                    step_side += STEP_SIDE;
                    break;
                case SDLK_d:
                    step_side -= STEP_SIDE;
                    break;
                default:
                    break;
                }
                break;
            case SDL_QUIT:
                quit = true;
                break;
            default:
                break;
            }
        }

        // ---------------------
        // Player movement
        // ---------------------

        vector_t _rot_dir = rotate_vector(player.dir, angle);
        player.dir = _rot_dir;

        vector_t _norm_dir = normalize_vector(_rot_dir);
        vector_t _orth_dir = get_orthogonal(_norm_dir);
        vector_t _new_forward = mult_vector(_norm_dir, step_forward);
        vector_t _new_side = mult_vector(_orth_dir, step_side);
        vector_t _new_dir = add_vector(_new_forward, _new_side);
        vector_t _new_pos = add_vector(player.pos, _new_dir);

        int _x = (int)_new_pos.x;
        int _y = (int)_new_pos.y;

        if (map[_y / TILE_HEIGHT][_x / TILE_WIDTH] == '.') {
            player.pos.x = _new_pos.x;
            player.pos.y = _new_pos.y;
        }

        prev_mouse_x = cur_mouse_x;
        prev_mouse_y = cur_mouse_y;

        // --------------------------
        // Framerate computation
        // --------------------------

        frame_ticks = SDL_GetTicks() - start_ticks;
        if (frame_ticks < screen_ticks_per_frame) {
            SDL_Delay(screen_ticks_per_frame - frame_ticks);
        }

        fps = 1000.0 / frame_ticks;
    }

    status = EXIT_SUCCESS;

Quit:
    SDL_FreeSurface(texture_img);
    SDL_DestroyTexture(gun_texture);
    SDL_FreeSurface(gun_surface);
    if (NULL != wall_texture) {
        SDL_DestroyTexture(wall_texture);
    }
    if (NULL != renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (NULL != main_window) {
        SDL_DestroyWindow(main_window);
    }
    SDL_Quit();
    return status;
}
