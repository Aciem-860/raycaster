#include <SDL2/SDL.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define TILE_WIDTH 64
#define TILE_HEIGHT 64
#define MAP_WIDTH 10
#define MAP_HEIGHT 10
#define STEP_FORWARD 20
#define STEP_SIDE 20
#define DELTA_TIME 10
#define ANGLE_STEP DEG_TO_RAG(10)
#define RAY_NUMBER 40.0

#define WW 640.0 // Window width
#define WH 480.0 // Window height
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

bool set_color(SDL_Renderer* renderer, SDL_Color color);
bool set_window_color(SDL_Renderer* renderer, SDL_Color color);
bool hit_x();
bool hit_y();
bool forward_x();
bool forward_y();

double absd(double v);
double norm2(vector_t* v);
double differential(vector_t* v);
double get_distance(vector_t* v1, vector_t* v2);
double dot_product(vector_t* v1, vector_t* v2);
double get_cos(vector_t* v1, vector_t* v2);

vector_t rotate_vector(vector_t* v, double angle);
vector_t get_orthogonal(vector_t* v);
vector_t add_vector(vector_t* v1, vector_t* v2);
vector_t sub_vector(vector_t* v1, vector_t* v2);
vector_t mult_vector(vector_t* v, double scalar);
vector_t normalize_vector(vector_t* v);
vector_t camera_segment(player_t* p);
vector_t find_next_point(vector_t* pos, vector_t* dir);

SDL_Color divide_by(SDL_Color c, int scalar);

// 0: Empty
// 1: Wall
static char map[MAP_WIDTH][MAP_HEIGHT] = {
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

static int cur_mouse_x, cur_mouse_y, prev_mouse_x, prev_mouse_y;

void load_map(const char* path) {
    FILE* map_file;
    map_file = fopen(path, "r");
    char line[MAP_WIDTH + 1];
    int row = 0;
    while (fgets(line, MAP_WIDTH + 1, map_file)) {
        if (strcmp(line, "\n")) {
            for (int col = 0; col < MAP_WIDTH; col++) {
                map[row][col] = line[col];
            }
            row++;
        }
    }

    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            printf("%c ", map[x][y]);
        }
        printf("\n");
    }

    fclose(map_file);
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

vector_t rotate_vector(vector_t* v, double angle) {
    vector_t ret = {cos(angle) * v->x + sin(angle) * v->y, -sin(angle) * v->x + cos(angle) * v->y};
    return ret;
}

vector_t get_orthogonal(vector_t* v) { return rotate_vector(v, M_PI / 2); }

vector_t add_vector(vector_t* v1, vector_t* v2) {
    vector_t ret = {v1->x + v2->x, v1->y + v2->y};
    return ret;
}

vector_t sub_vector(vector_t* v1, vector_t* v2) {
    vector_t opposite = mult_vector(v2, -1);
    return add_vector(v1, &opposite);
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
    vector_t rotated_dir = mult_vector(&norm_rotated_dir, 25 * tan(FOVR / 2));

    return rotated_dir;
}

double differential(vector_t* v) { return v->y / v->x; }

SDL_Color divide_by(SDL_Color c, int scalar) {
    SDL_Color _ret = {c.r / scalar, c.g / scalar, c.b / scalar, c.a};
    return _ret;
}

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
    double _x, _y;

    vector_t dx;
    vector_t dy;

    cx = dir->x > 0 ? 1 : -1;
    cy = dir->y > 0 ? 1 : -1;

    // ---------- //
    //   X axis   //
    // ---------- //

    if (dir->x > 0) { // Oriented to right
        _x = TILE_WIDTH - (int)(pos->x) % TILE_WIDTH;
    } else { // Oriented to left
        _x = -(int)(pos->x) % TILE_WIDTH;
        if (_x == 0)
            _x = -TILE_WIDTH;
    }
    vector_t _dx = {_x, _x * dydx};
    dx = _dx;

    if (dir->y > 0) { // Oriented to down
        _y = TILE_HEIGHT - (int)(pos->y) % TILE_HEIGHT;
    } else { // Oriented to top
        _y = -(int)(pos->y) % TILE_HEIGHT;
        if (_y == 0)
            _y = -TILE_HEIGHT;
    }
    vector_t _dy = {_y / dydx, _y};
    dy = _dy;

    if (norm2(&dy) < norm2(&dx)) {
        hity = true;
        hitx = false;
        return add_vector(pos, &dy);
    } else {
        hity = false;
        hitx = true;
        return add_vector(pos, &dx);
    }
}

double get_distance(vector_t* v1, vector_t* v2) {
    vector_t _diff = sub_vector(v2, v1);
    return norm2(&_diff);
}

double dot_product(vector_t* v1, vector_t* v2) { return v1->x * v2->x + v1->y * v2->y; }

double get_cos(vector_t* v1, vector_t* v2) { return dot_product(v1, v2) / (norm2(v1) * norm2(v2)); }

double absd(double v) {
    int sgn = v > 0 ? 1 : -1;
    return sgn * v;
}

int side = 0; // 0 = horizontal ; 1 = vertical

int main(int argc, char* argv[]) {
    load_map("../map");
    double angle = 0; // Angle made by dir vector with horizontal axis (left to right)
    double step_forward, step_side;
    vector_t cam_seg;

    SDL_Window* main_window = NULL;
    SDL_Renderer* renderer = NULL;

    SDL_Window* top_window = NULL;
    SDL_Renderer* top_renderer = NULL;

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

    top_window = SDL_CreateWindow("Top Down View", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  640, 640, SDL_WINDOW_SHOWN);
    if (NULL == top_window) {
        fprintf(stderr, "Error on SDL_CreateWindow: %s", SDL_GetError());
        goto Quit;
    }

    top_renderer = SDL_CreateRenderer(top_window, -1, SDL_RENDERER_ACCELERATED);
    if (NULL == top_renderer) {
        fprintf(stderr, "Error on SDL_CreateRenderer: %s", SDL_GetError());
        goto Quit;
    }

    SDL_Event event;
    bool quit = false;

    SDL_GetMouseState(&cur_mouse_x, &cur_mouse_y);
    prev_mouse_x = cur_mouse_x;
    prev_mouse_y = cur_mouse_y;

    while (!quit) {
        SDL_GetMouseState(&cur_mouse_x, &cur_mouse_y);
        set_window_color(renderer, black);
        set_window_color(top_renderer, black);

        int mouse_delta = cur_mouse_x - prev_mouse_x;
        printf("delta: %d\n", mouse_delta);

        // ------------------
        // Map Rendering
        // ------------------

        set_color(top_renderer, gray);
        for (int x = 0; x < MAP_WIDTH; x++) {
            for (int y = 0; y < MAP_HEIGHT; y++) {
                if (map[x][y] != '.') {
                    SDL_Rect _wall = {x * TILE_WIDTH, y * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT};
                    SDL_RenderFillRect(top_renderer, &_wall);
                }
            }
        }

        set_color(top_renderer, red);
        for (int x = 0; x < MAP_WIDTH; x++) {
            for (int y = 0; y < MAP_HEIGHT; y++) {
                SDL_RenderDrawLine(top_renderer, x * TILE_WIDTH, 0, x * TILE_WIDTH, 640);
                SDL_RenderDrawLine(top_renderer, 0, y * TILE_HEIGHT, WW, y * TILE_HEIGHT);
            }
        }

        // ------------------
        // RAYCASTING
        // ------------------

        cam_seg = camera_segment(&player);
        // Launch one ray by column
        for (int x = 0; x < WW; x++) {
            set_color(renderer, orange);
            double _frac = -((2.0 * x / WW) - 1);
            vector_t _cam_ray = mult_vector(&cam_seg, _frac);
            vector_t _ray = add_vector(&(player.dir), &_cam_ray);
            vector_t _hit = find_next_point(&player.pos, &_ray); // Intersection with grid

            vector_t p = player.pos;
            int _col, _row;
            for (int i = 0; i < 30; i++) {
                _hit = find_next_point(&p, &_ray);
                set_color(top_renderer, yellow);

                _col = (int)_hit.x / TILE_WIDTH;
                _row = (int)_hit.y / TILE_HEIGHT;

                // Start by correction if a ray hits the top-left corner
                if ((int)_hit.x % TILE_WIDTH == 0 && (int)_hit.y % TILE_HEIGHT == 0 &&
                    map[_col][_row] == '.') {
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

                if (map[_col][_row] != '.') {
                    break;
                }
                p = _hit;
            }

            // ----------------------
            // SHADING THE WALLS
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

            if (side) {
                set_color(top_renderer, yellow);
            } else if (hit_y()) {
                set_color(top_renderer, blue);
            }
            SDL_Rect hit_rect = {_hit.x, _hit.y, 2, 2};
            SDL_RenderFillRect(top_renderer, &hit_rect);
            // -------------------------------
            // Rendering the wall stripe
            // -------------------------------

            SDL_Color wall_color;
            switch (map[_col][_row]) {
            case 'b':
                wall_color = blue;
                break;
            case 'r':
                wall_color = red;
                break;
            case 'g':
                wall_color = green;
                break;
            }

            set_color(renderer, wall_color);
            if (side) {
                set_color(renderer, wall_color);
            } else {
                SDL_Color _c = divide_by(wall_color, 2);
                set_color(renderer, _c);
            }
            double _distance = get_distance(&player.pos, &_hit);
            double _corrected_distance = get_cos(&_ray, &player.dir) * _distance;
            double _wall_height = 120 * WH / _corrected_distance;
            SDL_RenderDrawLine(renderer, x, (WH - _wall_height) / 2, x, (WH + _wall_height) / 2);

            // SDL_Rect hit_block = {_col * TILE_WIDTH, _row * TILE_HEIGHT, TILE_WIDTH,
            // TILE_HEIGHT};
            // set_color(top_renderer, purple);
            // SDL_RenderDrawLine(top_renderer, player.pos.x, player.pos.y, _hit.x, _hit.y);
            // SDL_RenderFillRect(renderer, &hit_block);
        }

        // ------------------
        // SDL Rendering
        // ------------------

        angle = 0;
        step_forward = 0;
        step_side = 0;

        // Player dot and direction
        set_color(top_renderer, yellow);
        SDL_Rect player_rect = {player.pos.x - 2.5, player.pos.y - 2.5, 5, 5};
        SDL_RenderFillRect(top_renderer, &player_rect);

        vector_t _dir_pos = add_vector(&player.pos, &player.dir);
        SDL_RenderDrawLine(top_renderer, player.pos.x, player.pos.y, _dir_pos.x, _dir_pos.y);

        SDL_RenderPresent(renderer);
        SDL_RenderPresent(top_renderer);
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

        if (map[(int)(_new_pos.x / TILE_WIDTH)][(int)(_new_pos.y / TILE_HEIGHT)] == '.') {
            player.pos.x = _new_pos.x;
            player.pos.y = _new_pos.y;
        }

        prev_mouse_x = cur_mouse_x;
        prev_mouse_y = cur_mouse_y;
        SDL_Delay(DELTA_TIME);
    }

    status = EXIT_SUCCESS;

Quit:
    if (NULL != top_renderer) {
        SDL_DestroyRenderer(top_renderer);
    }
    if (NULL != top_window) {
        SDL_DestroyWindow(top_window);
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
