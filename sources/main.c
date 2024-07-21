#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#define TILE_WIDTH 64
#define TILE_HEIGHT 64
#define MAP_WIDTH 10
#define MAP_HEIGHT 10
#define STEP_FORWARD 5
#define STEP_SIDE 5
#define DELTA_TIME 10
#define ANGLE_STEP DEG_TO_RAG(5)
#define WALL_THICKNESS 20
#define RAY_NUMBER 40.0

#define WW 640   // Window width
#define WH 640   // Window height
#define FOV 66.0 // Field of view in degrees
#define DEG_TO_RAG(x) (x * M_PI / 180)
#define FOVR DEG_TO_RAG(FOV)

typedef struct {
    double x;
    double y;
} vector_t;

typedef struct {
    vector_t pos;
    vector_t dir;
} player_t;

// 0: Empty
// 1: Wall
static const int map[MAP_WIDTH][MAP_HEIGHT] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {1, 0, 0, 0, 0, 1, 0, 0, 0, 1}, {1, 0, 0, 0, 1, 1, 1, 0, 0, 1},
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 1}, {1, 1, 0, 0, 0, 0, 0, 1, 1, 1}, {1, 1, 0, 0, 0, 1, 1, 1, 0, 1},
    {1, 0, 0, 1, 1, 1, 0, 0, 0, 1}, {1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

static const SDL_Color white = {0xff, 0xff, 0xff, 0xff};
static const SDL_Color gray = {0x80, 0x80, 0x80, 0xff};
static const SDL_Color black = {0x00, 0x00, 0x00, 0xff};
static const SDL_Color red = {0xff, 0x00, 0x00, 0xff};
static const SDL_Color green = {0x00, 0xff, 0x00, 0xff};
static const SDL_Color blue = {0x00, 0x00, 0xff, 0xff};
static const SDL_Color purple = {0x80, 0x00, 0x80, 0xff};
static const SDL_Color yellow = {0xff, 0xff, 0x00, 0xff};
static const SDL_Color orange = {0xff, 0xa5, 0x00, 0xff};

static const vector_t i_pos = {100, 100};
static const vector_t i_dir = {50, 0};
static player_t player = {i_pos, i_dir};

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

vector_t rotate_vector(vector_t* v, double angle) {
    vector_t ret = {cos(angle) * v->x + sin(angle) * v->y, -sin(angle) * v->x + cos(angle) * v->y};
    return ret;
}

vector_t get_orthogonal(vector_t* v) { return rotate_vector(v, M_PI / 2); }

vector_t add_vector(vector_t* v1, vector_t* v2) {
    vector_t ret = {v1->x + v2->x, v1->y + v2->y};
    return ret;
}

vector_t mult_vector(vector_t* v, double scalar) {
    vector_t ret = {scalar * v->x, scalar * v->y};
    return ret;
}

double norm2(vector_t* v) {
    double x2 = pow(v->x, 2.0);
    double y2 = pow(v->y, 2.0);
    return sqrt(x2 + y2);
}

vector_t normalize_vector(vector_t* v) {
    double norm = norm2(v);
    vector_t ret = {v->x / norm, v->y / norm};
    return ret;
}

vector_t camera_segment(player_t* p) {
    vector_t raw_rotated_dir = get_orthogonal(&(p->dir));
    vector_t norm_rotated_dir = normalize_vector(&raw_rotated_dir);
    vector_t rotated_dir = mult_vector(&norm_rotated_dir, 100 * tan(FOVR / 2));

    return rotated_dir;
}

double differential(vector_t* v) { return v->y / v->x; }

// ---------------------------
// Raycasting methods
// ---------------------------

static int cx;
static int cy;

// These two variables are mutually exclusive
static bool hitx;
static bool hity;

bool hit_x() { return hitx; }
bool hit_y() { return hity; }

bool forward_x() { return cx == 1 ? true : false; }
bool forward_y() { return cy == 1 ? true : false; }

vector_t find_next_point(vector_t* pos, vector_t* dir) {
    double dydx = differential(dir);

    cx = dir->x > 0 ? 1 : -1;
    cy = dir->y > 0 ? 1 : -1;

    // Compute for X axis
    double dX = (int)(pos->x) % TILE_WIDTH;
    if (cx == 1 || (cx == -1 && dX == 0)) {
        dX = TILE_WIDTH - dX;
    }

    double reached_y = cx * dX * dydx;
    vector_t _x = {cx * dX, reached_y};
    vector_t candidate_x = add_vector(pos, &_x);

    // Compute for Y axis
    double dY = (int)(pos->y) % TILE_HEIGHT;
    if (cy > 0 || (cy == -1 && dY == 0)) {
        dY = TILE_HEIGHT - dY;
    }
    double reached_x = cy * dY / dydx;
    vector_t _y = {reached_x, cy * dY};
    vector_t candidate_y = add_vector(pos, &_y);

    double dist_x = norm2(&_x);
    double dist_y = norm2(&_y);

    if (dist_y < dist_x) {
        hity = true;
        hitx = false;
        return candidate_y;
    } else {
        hitx = true;
        hity = false;
        return candidate_x;
    }
}

int main(int argc, char* argv[]) {
    double angle = 0; // Angle made by dir vector with horizontal axis (left to right)
    double step_forward, step_side;
    vector_t cam_seg;

    SDL_Window* main_window = NULL;
    SDL_Renderer* renderer = NULL;
    int status = EXIT_FAILURE;

    if (0 != SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Error on SDL_Init: %s", SDL_GetError());
        goto Quit;
    }

    main_window = SDL_CreateWindow("Top Down View", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   WW, WH, SDL_WINDOW_SHOWN);
    if (NULL == main_window) {
        fprintf(stderr, "Error on SDL_CreateWindow: %s", SDL_GetError());
        goto Quit;
    }

    renderer = SDL_CreateRenderer(main_window, -1, SDL_RENDERER_ACCELERATED);
    if (NULL == renderer) {
        fprintf(stderr, "Error on SDL_CreateRenderer: %s", SDL_GetError());
        goto Quit;
    }

    SDL_Event event;
    bool quit = false;

    while (!quit) {
        set_window_color(renderer, black);

        // ------------------
        // Map Rendering
        // ------------------

        // set_color(renderer, gray);
        // for (int x = 0; x < MAP_WIDTH; x++) {
        //     for (int y = 0; y < MAP_HEIGHT; y++) {
        //         if (map[x][y]) {
        //             SDL_Rect _wall = {x * TILE_WIDTH, y * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT};
        //             SDL_RenderFillRect(renderer, &_wall);
        //         }
        //     }
        // }

        // ------------------
        // RAYCASTING
        // ------------------

        cam_seg = camera_segment(&player);
        // Launch one ray by column
        for (int x = 0; x < WW; x++) {
            set_color(renderer, orange);
            double _frac = (2.0 * x / WW) - 1;
            vector_t _cam_ray = mult_vector(&cam_seg, _frac);
            vector_t _ray = add_vector(&(player.dir), &_cam_ray);
            vector_t _hit = find_next_point(&player.pos, &_ray); // Intersection with grid

            vector_t p = player.pos;
            int _col, _row;
            for (int i = 0; i < 30; i++) {
                _hit = find_next_point(&p, &_ray);
                if (hit_x()) {
                    if (forward_x()) {
                        _col = (int)_hit.x / TILE_WIDTH;
                        _row = (int)_hit.y / TILE_HEIGHT;
                    } else {
                        _col = ((int)_hit.x - 1) / TILE_WIDTH;
                        _row = (int)_hit.y / TILE_HEIGHT;
                    }
                } else if (hit_y()) {
                    if (forward_y()) {
                        _col = (int)_hit.x / TILE_WIDTH;
                        _row = (int)_hit.y / TILE_HEIGHT;
                    } else {
                        _col = (int)_hit.x / TILE_WIDTH;
                        _row = ((int)_hit.y - 1) / TILE_HEIGHT;
                    }
                } else {
                    fprintf(stderr, "[ ERROR ] Should have hit something\n");
                    exit(EXIT_FAILURE);
                }

                if (map[_col][_row] != 0) {
                    break;
                }
                p = _hit;
            }
            // SDL_Rect hit_block = {_col * TILE_WIDTH, _row * TILE_HEIGHT, TILE_WIDTH,
            // TILE_HEIGHT}; SDL_RenderDrawLine(renderer, player.pos.x, player.pos.y, _hit.x,
            // _hit.y); set_color(renderer, purple); SDL_RenderFillRect(renderer, &hit_block);
        }

        // ------------------
        // SDL Rendering
        // ------------------

        angle = 0;
        step_forward = 0;
        step_side = 0;

        // Player dot and direction
        // set_color(renderer, yellow);
        // SDL_Rect player_rect = {player.pos.x - 2.5, player.pos.y - 2.5, 5, 5};
        // SDL_RenderFillRect(renderer, &player_rect);

        // vector_t lengthened_dir = mult_vector(&player.dir, 25);
        // vector_t dir_pos = add_vector(&player.pos, &lengthened_dir);

        // vector_t dir_pos = add_vector(&player.pos, &player.dir);
        // SDL_RenderDrawLine(renderer, player.pos.x, player.pos.y, dir_pos.x, dir_pos.y);

        SDL_RenderPresent(renderer);
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_x:
                    quit = true;
                    break;
                case SDLK_a:
                    angle += ANGLE_STEP;
                    break;
                case SDLK_e:
                    angle -= ANGLE_STEP;
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

        vector_t _rot_dir = rotate_vector(&player.dir, angle);
        player.dir = _rot_dir;

        vector_t _norm_dir = normalize_vector(&_rot_dir);
        vector_t _orth_dir = get_orthogonal(&_norm_dir);
        vector_t _new_forward = mult_vector(&_norm_dir, step_forward);
        vector_t _new_side = mult_vector(&_orth_dir, step_side);
        vector_t _new_dir = add_vector(&_new_forward, &_new_side);
        vector_t _new_pos = add_vector(&player.pos, &_new_dir);
        player.pos.x = _new_pos.x;
        player.pos.y = _new_pos.y;

        SDL_Delay(DELTA_TIME);
    }

    status = EXIT_SUCCESS;

Quit:
    if (NULL != renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (NULL != main_window) {
        SDL_DestroyWindow(main_window);
    }
    SDL_Quit();
    return status;
}
