#include "body.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

double MAX_ROT_VELOCITY = 0.15;

typedef struct body {
  list_t *shape;
  double mass;
  rgb_color_t color;
  double angle;
  vector_t velocity;
  double rot_velocity;
  vector_t centroid;
  vector_t net_force;
  vector_t net_impulse;
  free_func_t info_freer;
  void *info;
  bool is_removed;
  bool is_destroyable;
  vector_t rotation_center;
  double rot_acceleration;
} body_t;

body_t *body_init(list_t *shape, double mass, rgb_color_t color) {
  body_t *body = malloc(sizeof(body_t));
  assert(mass > 0);
  *body = (body_t){.shape = shape,
                   .mass = mass,
                   .color = color,
                   .angle = 0,
                   .velocity = VEC_ZERO,
                   .net_force = VEC_ZERO,
                   .net_impulse = VEC_ZERO,
                   .rot_velocity = 0,
                   .rot_acceleration = 0};
  assert(body != NULL);
  return body;
}

body_t *body_init_with_info(list_t *shape, double mass, rgb_color_t color,
                            void *info, free_func_t info_freer) {
  body_t *body = body_init(shape, mass, color);
  body->info = info;
  body->info_freer = info_freer;

  return body;
};

void body_free(body_t *body) {
  list_free(body->shape);
  if (body->info_freer != NULL) {
    body->info_freer(body->info);
  }
  free(body);
}

list_t *body_get_shape(body_t *body) {
  list_t *return_shape = list_init(list_size(body->shape), (free_func_t)free);
  for (size_t i = 0; i < list_size(body->shape); i++) {
    vector_t *return_vec = malloc(sizeof(vector_t));
    return_vec->x = ((vector_t *)list_get(body->shape, i))->x;
    return_vec->y = ((vector_t *)list_get(body->shape, i))->y;
    list_add(return_shape, return_vec);
  }
  return return_shape;
}

double body_area_helper(list_t *shape) {
  double area = 0;
  size_t size = list_size(shape);
  for (size_t i = 0; i < size; i++) { // Mod is to loop around for shoelace
    area += ((vector_t *)list_get(shape, i))->x *
            ((vector_t *)list_get(shape, (i + 1) % size))->y;
    area -= ((vector_t *)list_get(shape, i))->y *
            ((vector_t *)list_get(shape, (i + 1) % size))->x;
  }
  area = area / 2;
  return area;
}

vector_t body_get_centroid(body_t *body) {

  vector_t centroid = {0.0, 0.0};
  size_t size = list_size(body->shape);
  for (size_t i = 0; i < size; i++) { // Mod is to loop around
    double xi = ((vector_t *)list_get(body->shape, i))->x;
    double xi_plus1 = ((vector_t *)list_get(body->shape, (i + 1) % size))->x;
    double yi = ((vector_t *)list_get(body->shape, i))->y;
    double yi_plus1 = ((vector_t *)list_get(body->shape, (i + 1) % size))->y;
    centroid.x += (xi + xi_plus1) * (xi * yi_plus1 - xi_plus1 * yi);
    centroid.y += (yi + yi_plus1) * (xi * yi_plus1 - xi_plus1 * yi);
  }

  centroid.x = centroid.x / (6 * body_area_helper(body->shape));
  centroid.y = centroid.y / (6 * body_area_helper(body->shape));
  return centroid;
}

double body_get_mass(body_t *body) { return body->mass; }

void *body_get_info(body_t *body) { return body->info; }

vector_t body_get_velocity(body_t *body) { return body->velocity; }

vector_t body_get_net_force(body_t *body) { return body->net_force; }

vector_t body_get_net_impulse(body_t *body) { return body->net_impulse; }

rgb_color_t body_get_color(body_t *body) { return body->color; }

double body_get_rot_velocity(body_t *body) { return body->rot_velocity; }

void body_set_centroid(body_t *body, vector_t x) {
  vector_t center_diff = vec_subtract(x, body_get_centroid(body));
  for (size_t i = 0; i < list_size(body->shape); i++) {
    *((vector_t *)list_get(body->shape, i)) =
        vec_add(*((vector_t *)list_get(body->shape, i)), center_diff);
  }
}

void body_set_velocity(body_t *body, vector_t v) { body->velocity = v; }

void body_set_rotation(body_t *body, double angle) {
  double new_angle = angle - body->angle;
  vector_t centroid = body_get_centroid(body);
  for (size_t i = 0; i < list_size(body->shape); i++) {
    vector_t diff =
        vec_subtract(*(vector_t *)list_get(body->shape, i), centroid);
    *(vector_t *)list_get(body->shape, i) = vec_rotate(diff, new_angle);
  }
  body->angle = angle;
  body_set_centroid(body, centroid);
}

void body_add_force(body_t *body, vector_t force) {
  body->net_force = vec_add(body->net_force, force);
}

void body_add_impulse(body_t *body, vector_t impulse) {
  body->net_impulse = vec_add(body->net_impulse, impulse);
}

void body_remove_all_forces(body_t *body) { body->net_force = VEC_ZERO; }

void body_remove_x_forces(body_t *body) {
  body->net_force = (vector_t){0.0, body_get_net_force(body).y};
}

double body_distance(body_t *body1, body_t *body2) {
  vector_t centroid1 = body_get_centroid(body1);
  vector_t centroid2 = body_get_centroid(body2);

  return sqrt(vec_dot(vec_subtract(centroid1, centroid2),
                      vec_subtract(centroid1, centroid2)));
}

void body_set_rotation_center(body_t *body, vector_t center) {
  body->rotation_center = center;
}

void body_rotate(body_t *body, double angle) {
  vector_t centroid = body_get_centroid(body);
  body->rotation_center = centroid;
  body_rotate_about(body, angle, centroid);
}

void body_rotate_about(body_t *body, double angle, vector_t point) {
  for (size_t i = 0; i < list_size(body->shape); i++) {
    vector_t diff = vec_subtract(*(vector_t *)list_get(body->shape, i), point);
    *(vector_t *)list_get(body->shape, i) = vec_rotate(diff, angle);
    *(vector_t *)list_get(body->shape, i) =
        vec_add(*(vector_t *)list_get(body->shape, i), point);
  }

  body->angle = fmod((body->angle + angle), (2 * M_PI));
}

double body_get_angle(body_t *body) { return body->angle; }

double body_get_rot_acceleration(body_t *body) {
  return body->rot_acceleration;
}

void body_set_shape(body_t *body, list_t *shape) { body->shape = shape; }

void body_add_vertex(body_t *body, vector_t *vector) {
  list_add(body->shape, vector);
}

void body_set_rot_velocity(body_t *body, double rot_velocity) {
  body->rot_velocity = rot_velocity;
  body->rotation_center = body_get_centroid(body);
}

void body_set_rot_acceleration(body_t *body, double rot_acceleration) {
  body->rot_acceleration = rot_acceleration;
}

void body_tick(body_t *body, double dt) {

  vector_t force_velocity = vec_multiply(dt / body->mass, body->net_force);
  vector_t net_velocity_change =
      vec_add(force_velocity, vec_multiply(1 / body->mass, body->net_impulse));
  vector_t avg_velocity =
      vec_average(body->velocity, vec_add(body->velocity, net_velocity_change));
  body->velocity = vec_add(body->velocity, net_velocity_change);

  vector_t new_center =
      vec_add(body_get_centroid(body), vec_multiply(dt, avg_velocity));
  body_set_centroid(body, new_center);

  if (body->rot_velocity < MAX_ROT_VELOCITY) {
    body->rot_velocity += dt * body->rot_acceleration;
  }
  body_rotate_about(body, body->rot_velocity, body->rotation_center);

  body->net_force = VEC_ZERO;
  body->net_impulse = VEC_ZERO;
}

void body_remove(body_t *body) { body->is_removed = true; }

bool body_is_removed(body_t *body) { return body->is_removed; }