#include "vector.h"
#include "math.h"

vector_t rotate_vector(vector_t v, double angle) {
    vector_t ret = {cos(angle) * v.x + sin(angle) * v.y, -sin(angle) * v.x + cos(angle) * v.y};
    return ret;
}

vector_t get_orthogonal(vector_t v) { return rotate_vector(v, M_PI / 2); }

vector_t add_vector(vector_t v1, vector_t v2) {
    vector_t ret = {v1.x + v2.x, v1.y + v2.y};
    return ret;
}

vector_t sub_vector(vector_t v1, vector_t v2) {
    vector_t opposite = mult_vector(v2, -1);
    return add_vector(v1, opposite);
}

vector_t mult_vector(vector_t v, double scalar) {
    vector_t ret = {scalar * v.x, scalar * v.y};
    return ret;
}

double norm2(vector_t v) {
    double x2 = pow(v.x, 2.0);
    double y2 = pow(v.y, 2.0);
    return sqrt(x2 + y2);
}

vector_t normalize_vector(vector_t v) {
    double norm = norm2(v);
    vector_t ret = {v.x / norm, v.y / norm};
    return ret;
}

vector_t camera_segment(player_t p) {
    vector_t raw_rotated_dir = get_orthogonal(p.dir);
    vector_t norm_rotated_dir = normalize_vector(raw_rotated_dir);
    // vector_t rotated_dir = mult_vector(&norm_rotated_dir, 25 * tan(FOVR / 2));

    return norm_rotated_dir;
}

double differential(vector_t v) { return v.y / v.x; }

double get_distance(vector_t v1, vector_t v2) {
    vector_t _diff = sub_vector(v2, v1);
    return norm2(_diff);
}

double dot_product(vector_t v1, vector_t v2) { return v1.x * v2.x + v1.y * v2.y; }

double get_cos(vector_t v1, vector_t v2) { return dot_product(v1, v2) / (norm2(v1) * norm2(v2)); }
