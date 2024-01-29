#include "vector.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const vector_t VEC_ZERO = {0.0, 0.0};

vector_t vec_multiply(double scalar, vector_t v) {
  vector_t v_mult = {scalar * v.x, scalar * v.y};
  return v_mult;
}

vector_t vec_negate(vector_t v) { return vec_multiply(-1, v); }

vector_t vec_add(vector_t v1, vector_t v2) {
  vector_t v_sum = {v1.x + v2.x, v1.y + v2.y};
  return v_sum;
}

vector_t vec_subtract(vector_t v1, vector_t v2) {
  return vec_add(v1, vec_negate(v2));
}

double vec_dot(vector_t v1, vector_t v2) { return v1.x * v2.x + v1.y * v2.y; }

double vec_cross(vector_t v1, vector_t v2) { return v1.x * v2.y - v1.y * v2.x; }

vector_t vec_rotate(vector_t v, double angle) {
  vector_t vec_rotate;
  vec_rotate.x = cos(angle) * v.x - sin(angle) * v.y;
  vec_rotate.y = sin(angle) * v.x + cos(angle) * v.y;
  return vec_rotate;
}

vector_t vec_average(vector_t v1, vector_t v2) {
  return (vector_t){(v1.x + v2.x) / 2, (v1.y + v2.y) / 2};
}

vector_t vec_unit_vector(vector_t v) {
  double norm = sqrt(vec_dot(v, v));
  return (vector_t){v.x / norm, v.y / norm};
}

double vec_length(vector_t v) { return sqrt(vec_dot(v, v)); }

double vec_component(vector_t v1, vector_t u1) { return vec_dot(v1, u1); }

bool vec_equals(vector_t v1, vector_t v2) {
  return (v1.x == v2.x) && (v1.y == v2.y);
}

void vec_print(vector_t v) { printf("(%f, %f)\n", v.x, v.y); }