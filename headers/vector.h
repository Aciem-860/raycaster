#ifndef VECTOR_H
#define VECTOR_H

typedef struct {
    double x;
    double y;
} vector_t;

typedef struct {
    vector_t pos;
    vector_t dir;
} player_t;

double norm2(vector_t v);
double differential(vector_t v);
double get_distance(vector_t v1, vector_t v2);
double dot_product(vector_t v1, vector_t v2);
double get_cos(vector_t v1, vector_t v2);

vector_t rotate_vector(vector_t v, double angle);
vector_t get_orthogonal(vector_t v);
vector_t add_vector(vector_t v1, vector_t v2);
vector_t sub_vector(vector_t v1, vector_t v2);
vector_t mult_vector(vector_t v, double scalar);
vector_t normalize_vector(vector_t v);
vector_t camera_segment(player_t p);

#endif
