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

// IDLE: not firing
// LOADING: start to fire (for minigun, not for gun, consists in 2 first frames)
// FIRING: firing animation
typedef enum { IDLE, LOADING, FIRING } gun_anim_state;
static gun_anim_state gun_state = IDLE;

// 64: door is fully closed
// 0 : door is fully opened
int door_timer = 64;

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

static bool door_opening = false;

// Returns true if it hits a door
bool get_wall_hit(vector_t pos, vector_t dir, double frac, bool ignore_door, vector_t* hit,
                  vector_t* ray, int* col, int* row) {
    vector_t _cam_ray = mult_vector(camera_segment(player), frac);
    vector_t _ray = add_vector((player.dir), _cam_ray);
    vector_t _hit = find_next_point(player.pos, _ray); // Intersection with grid
    int _col, _row;
    vector_t p = player.pos;
    bool ret = false;

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

        if (map[_row][_col] == 'p' && !ignore_door) {
            ret = true;
            break;
        } else if (map[_row][_col] != '.' && map[_row][_col] != 'p') {
            break;
        }
        p = _hit;
    }
    *ray = _ray;
    *hit = _hit;
    *col = _col;
    *row = _row;
    return ret;
}

int start() {

    // ---------------------
    // SDL Initializing
    // ---------------------

    SDL_Window* main_window = NULL;
    SDL_Renderer* renderer = NULL;

    SDL_Surface* texture_img = IMG_Load(textures_path);
    SDL_Surface* gun_surface = IMG_Load("../minigun.png");

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
    // Animation
    // --------------------------

    int anim_frame = 0;

    // --------------------------
    // Raycasting parameters
    // --------------------------

    double angle = 0; // Angle made by dir vector with horizontal axis (left to right)
    double step_forward, step_side;
    vector_t cam_seg;

    // Loading the map
    load_map("../map");
    load_sprite_map("../sprite_map");

    // Number of frame before remove 1 bullet
    int ammo_cpt = 0;
    int dmg = 1; // Weapon damage

    // ---------------------------
    // Preload props textures
    // ---------------------------

    SDL_Surface* wooden_barrel_surf = IMG_Load(wooden_barrel_sprite.path);
    SDL_Surface* iron_barrel_surf = IMG_Load(iron_barrel_sprite.path);
    SDL_Surface* dinner_table_surf = IMG_Load(dinner_table_sprite.path);
    SDL_Surface* well_water_surf = IMG_Load(well_water_sprite.path);
    SDL_Surface* armor_surf = IMG_Load(armor_sprite.path);
    SDL_Surface* furnace_surf = IMG_Load(furnace_sprite.path);
    SDL_Surface* pillar_surf = IMG_Load(pillar_sprite.path);
    SDL_Surface* soldier_surf = IMG_Load(soldier_sprite.path);

    SDL_Texture* wooden_barrel_text = SDL_CreateTextureFromSurface(renderer, wooden_barrel_surf);
    SDL_Texture* iron_barrel_text = SDL_CreateTextureFromSurface(renderer, iron_barrel_surf);
    SDL_Texture* dinner_table_text = SDL_CreateTextureFromSurface(renderer, dinner_table_surf);
    SDL_Texture* well_water_text = SDL_CreateTextureFromSurface(renderer, well_water_surf);
    SDL_Texture* armor_text = SDL_CreateTextureFromSurface(renderer, armor_surf);
    SDL_Texture* furnace_text = SDL_CreateTextureFromSurface(renderer, furnace_surf);
    SDL_Texture* pillar_text = SDL_CreateTextureFromSurface(renderer, pillar_surf);
    SDL_Texture* soldier_text = SDL_CreateTextureFromSurface(renderer, soldier_surf);

    // --------------------
    // Main game loop
    // --------------------

    while (!quit) {

        if (door_opening) {
            door_timer -= 1;
            if (door_timer < 0) {
                door_timer = 0;
                door_opening = false;
            }
        }

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
                int tx_cl = 10 * TEXTURE_WIDTH + (int)floor.x % TEXTURE_WIDTH;
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
            int _col, _row;

            vector_t _hit = {0, 0};
            vector_t _ray = {0, 0};
            bool door_tile =
                get_wall_hit(player.pos, player.dir, _frac, false, &_hit, &_ray, &_col, &_row);

            // ----------------------
            // Door rendering
            // ----------------------

            bool door = false;
            bool _hitx = hit_x(); // Need to use a copy of hitx because the call to find_next_point
                                  // in the if statement modifies the global variable named hitx

            if (door_tile) {
                // vector_t ray = sub_vector(_hit, player.pos);
                vector_t wall_hit = {0, 0};

                // vector_t _ray;
                int __col, __row;

                get_wall_hit(player.pos, _ray, _frac, true, &wall_hit, &_ray, &__col, &__row);

                //  vector_t wall_hit = find_next_point(_hit, ray);
                vector_t door_hit = {0, 0};
                double slope = differential(_ray);
                double dx, dy, dw, dd;

                if (_hitx) {
                    dx = cx * (double)TILE_WIDTH / 2;
                    dy = dx * slope;
                } else {
                    dy = cy * (double)TILE_WIDTH / 2;
                    dx = dy / slope;
                }
                door_hit.x = _hit.x + dx;
                door_hit.y = _hit.y + dy;
                dd = get_distance(player.pos, door_hit);
                dw = get_distance(player.pos, wall_hit);

                if (dd < dw) { // Render door
                    // Check if the door hit is open
                    int _length;
                    if (_hitx) {
                        _length = ((int)door_hit.y % TILE_HEIGHT);
                    } else {
                        _length = ((int)door_hit.x % TILE_WIDTH);
                    }
                    if (_length <= door_timer) {
                        _hit = door_hit;
                        door = true;
                    } else {
                        door_tile = false;
                        door = false;
                        _hit = wall_hit;
                        _col = __col;
                        _row = __row;
                    }
                } else { // Render wall
                    _hit = wall_hit;
                    door = false;
                }
            }

            // ----------------------
            // Shading the walls
            // ----------------------

            int _x = (int)_hit.x;
            int _y = (int)_hit.y;
            int _xmod, _ymod;

            if (door_tile && door) {
                if (_hitx) {
                    _xmod = _x % (TILE_WIDTH / 2);
                    _ymod = _y % TILE_HEIGHT;
                } else {
                    _xmod = _x % TILE_WIDTH;
                    _ymod = _y % (TILE_HEIGHT / 2);
                }
            } else {
                _xmod = _x % TILE_WIDTH;
                _ymod = _y % TILE_HEIGHT;
            }

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
                _text_offset = 1;
                break;
            case 'f': // Brick Wall with flag
                _text_offset = 0;
                break;
            case 's': // Stone Wall
                _text_offset = 3;
                break;
            case 'g': // Blue Brick
                _text_offset = 4;
                break;
            case 'w': // Wooden wall
                _text_offset = 6;
                break;
            case 'm': // Mossy Stone Wall
                _text_offset = 5;
                break;
            case 't': // Terracota Wall
                _text_offset = 7;
                break;
            case 'p': // Door
                if (door)
                    _text_offset = 8;
                else
                    _text_offset = 9;
                break;
            }
            /* if (door_tile && door) {
                _text_offset = 8;
            } */

            _text_offset *= TEXTURE_WIDTH;

            double _distance = get_distance(player.pos, _hit);
            double _orthogonal_distance = get_cos(_ray, player.dir) * _distance;
            double _wall_height = 64 * WH / _orthogonal_distance;
            double _frac_text = side == 0 ? _xmod : _ymod;
            wall_distance[x] = _orthogonal_distance;

            SDL_Rect src = {_text_offset + _frac_text, 0, 1, TEXTURE_HEIGHT};

            if (door) {
                src.x += 64 - door_timer;
            }

            SDL_Rect dst = {x, (WH - _wall_height) / 2, 1, _wall_height};
            SDL_RenderCopy(renderer, wall_texture, &src, &dst);
            if (!side) {
                SDL_Color _c = {0x0, 0x0, 0x0, 0x80};
                set_color(renderer, _c);
                SDL_RenderDrawLine(renderer, x, (WH - _wall_height) / 2, x,
                                   (WH + _wall_height) / 2);
            }
        }

        // ---------------------------
        // Rendering props & enemies
        // ---------------------------

        SDL_Texture* prop_texture;

        // Contains both props and enemies
        real_world_prop_t props_to_render[prop_number];
        int _nb_props = 0; // Number of props to render

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

                switch (_prop.type) {
                case WOODEN_BARREL:
                    prop_texture = wooden_barrel_text;
                    break;
                case IRON_BARREL:
                    prop_texture = iron_barrel_text;
                    break;
                case DINNER_TABLE:
                    prop_texture = dinner_table_text;
                    break;
                case FURNACE:
                    prop_texture = furnace_text;
                    break;
                case ARMOR:
                    prop_texture = armor_text;
                    break;
                case WELLWATER:
                    prop_texture = well_water_text;
                    break;
                case PILLAR:
                    prop_texture = pillar_text;
                    break;
                case SOLDIER:
                    prop_texture = soldier_text;
                    break;
                default:
                    fprintf(stderr, "empty prop\n");
                    break;
                }

                SDL_SetRenderTarget(renderer, prop_texture);

                for (int x = 0; x < w; x++) {
                    int _x = x * 64 / w;
                    if (orth_distance < wall_distance[(int)(WW / 2 - x_offset - w / 2 + x)]) {
                        SDL_Rect _src = {_x, 0, 1, TILE_HEIGHT};
                        if (_prop.type == SOLDIER && _prop.state == PROP_DEAD) {
                            _src.x += 4 * 64;
                            _src.y += 5 * 64;
                        }
                        SDL_Rect _dst = {WW / 2 - x_offset - w / 2 + x, WH / 2 - h / 2, 1, h};
                        SDL_RenderCopy(renderer, prop_texture, &_src, &_dst);
                    }
                }
            }
        }

        // ---------------------
        // Rendering gun
        // ---------------------

        const int gun_w = 500;
        const int gun_h = 500;
        SDL_SetRenderTarget(renderer, gun_texture);

        int factor = 5;
        int offset = 0;
        int nb_frame = 4;

        if (is_firing && ammo) {
            offset = anim_frame / factor;
            if (gun_state == IDLE) {
                gun_state = LOADING;
            } else if (gun_state == LOADING && offset == 3) {
                gun_state = FIRING;
            } else if (gun_state == FIRING) {
                offset += 2;
                nb_frame = 2;
            }
            ammo_cpt++;
            if (ammo_cpt == 3) {
                ammo--;
                ammo_cpt = 0;
            }
        } else {
            gun_state = IDLE;
        }

        SDL_Rect gun_src = {offset * 128, 0, 128, 128};
        SDL_Rect gun_dst = {(WW - gun_w) / 2, WH - gun_h + 100, gun_w, gun_h};
        SDL_RenderCopy(renderer, gun_texture, &gun_src, &gun_dst);

        anim_frame = (anim_frame + 1) % (nb_frame * factor);

        // ------------------------------------
        // Check if an enemy has been hit
        // ------------------------------------

        // Check only visible props
        if (is_firing && gun_state == FIRING) {
            for (int i = 0; i < 100; i++) {
                if (enemy_index[i] == -1) {
                    break;
                }

                prop_t* prop = &props[enemy_index[i]];
                if (prop->state == PROP_DEAD) {
                    continue;
                }
                vector_t ray = sub_vector(prop->position, player.pos);
                double cs = get_cos(ray, player.dir);
                double dist = cs * norm2(ray);
                if (dist < wall_distance[(int)WW / 2]) {
                    if (100 * cs >= 100 - 0.1) { // An enemy has been hit
                        if (prop->life > 0) {
                            prop->life -= dmg;
                        } else {
                            prop->state = PROP_DEAD;
                        }
                    }
                }
            }
        }

        // ---------------------
        // Framerate printing
        // ---------------------

        SDL_Surface* text;
        char* framerate_txt;
        asprintf(&framerate_txt, "FPS: %d              AMMO: %d", (int)fps, ammo);
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
                case SDLK_k:
                    ammo = 100;
                    break;
                case SDLK_u:
                    door_opening = true;
                    false;
                default:
                    break;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                is_firing = true;
                break;
            case SDL_MOUSEBUTTONUP:
                is_firing = false;
                break;
            case SDL_QUIT:
                quit = true;
                break;
            default:
                break;
            }
        }

        // ------------------------------------
        // Player movement and collision
        // ------------------------------------

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

        prop_t* collided_prop = sprite_at_pos(_x, _y);
        bool collided = false;
        if (collided_prop != EMPTY) {
            sprite_t sprite = get_sprite(collided_prop->type);
            collided = sprite.collision && collided_prop->state != PROP_DEAD;
        }

        if (map[_y / TILE_HEIGHT][_x / TILE_WIDTH] == '.' && !collided) {
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
    SDL_FreeSurface(wooden_barrel_surf);
    SDL_FreeSurface(iron_barrel_surf);
    SDL_FreeSurface(dinner_table_surf);
    SDL_FreeSurface(well_water_surf);
    SDL_FreeSurface(armor_surf);
    SDL_FreeSurface(furnace_surf);
    SDL_FreeSurface(pillar_surf);
    SDL_FreeSurface(soldier_surf);

    SDL_DestroyTexture(well_water_text);
    SDL_DestroyTexture(wooden_barrel_text);
    SDL_DestroyTexture(iron_barrel_text);
    SDL_DestroyTexture(dinner_table_text);
    SDL_DestroyTexture(armor_text);
    SDL_DestroyTexture(furnace_text);
    SDL_DestroyTexture(pillar_text);
    SDL_DestroyTexture(soldier_text);

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
