#include "force_creator.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct force_aux_2bodies {
  double Constant;
  body_t *body1;
  body_t *body2;
} force_aux_2bodies_t;

typedef struct force_aux_1body {
  double Constant;
  body_t *body;
} force_aux_1body_t;

typedef struct force_aux_collision_bodies {
  body_t *body1;
  body_t *body2;
} force_aux_collision_bodies_t;

typedef struct force_aux_collision {
  collision_handler_t handler;
  body_t *body1;
  body_t *body2;
  void *collision_aux;
  free_func_t freer;
  bool are_colliding;
} force_aux_collision_t;

typedef struct collision_aux_destructive {
  bool body1_is_destroyable;
  bool body2_is_destroyable;
  size_t coll_before_destruct;
} collision_aux_destructive_t;

typedef struct collision_aux_physics {
  double elasticity;
} collision_aux_physics_t;

force_aux_1body_t *force_aux_1body_init(double constant, body_t *body) {
  force_aux_1body_t *aux = malloc(sizeof(force_aux_1body_t));
  aux->Constant = constant;
  aux->body = body;
  return aux;
}

force_aux_2bodies_t *force_aux_2bodies_init(double constant, body_t *body1,
                                            body_t *body2) {
  force_aux_2bodies_t *aux = malloc(sizeof(force_aux_2bodies_t));
  aux->Constant = constant;
  aux->body1 = body1;
  aux->body2 = body2;
  return aux;
}

force_aux_collision_bodies_t *force_aux_collision_bodies_init(body_t *body1,
                                                              body_t *body2) {
  force_aux_collision_bodies_t *aux =
      malloc(sizeof(force_aux_collision_bodies_t));
  aux->body1 = body1;
  aux->body2 = body2;
  return aux;
}

force_aux_collision_t *force_aux_collision_init(body_t *body1, body_t *body2,
                                                collision_handler_t handler,
                                                void *aux, free_func_t freer) {
  force_aux_collision_t *collision_aux = malloc(sizeof(force_aux_collision_t));
  collision_aux->body1 = body1;
  collision_aux->body2 = body2;
  collision_aux->handler = handler;
  collision_aux->collision_aux = aux;
  collision_aux->freer = freer;
  collision_aux->are_colliding = false;
  return collision_aux;
}

collision_aux_physics_t *collision_aux_physics_init(double elasticity) {
  collision_aux_physics_t *aux = malloc(sizeof(collision_aux_physics_t));
  aux->elasticity = elasticity;
  return aux;
}

collision_aux_destructive_t *
collision_aux_destructive_init(bool body1_is_destroyable,
                               bool body2_is_destroyable,
                               size_t coll_before_destruct) {
  collision_aux_destructive_t *aux =
      malloc(sizeof(collision_aux_destructive_t));
  aux->body1_is_destroyable = body1_is_destroyable;
  aux->body2_is_destroyable = body2_is_destroyable;
  aux->coll_before_destruct = coll_before_destruct;
  return aux;
}

void calc_gravity(void *void_aux) {
  force_aux_2bodies_t *aux = (force_aux_2bodies_t *)void_aux;
  double G = aux->Constant;
  body_t *body1 = aux->body1;
  body_t *body2 = aux->body2;

  vector_t diff =
      vec_subtract(body_get_centroid(body1), body_get_centroid(body2));
  double distance = body_distance(body1, body2);
  if (distance < 5) {
    distance = 5;
  }
  vector_t unit_vector = vec_multiply(1 / distance, diff);
  vector_t force_1on2 = vec_multiply(
      G * body_get_mass(body1) * body_get_mass(body2) / (distance * distance),
      unit_vector);

  body_add_force(body1, vec_negate(force_1on2));
  body_add_force(body2, force_1on2);
}

void calc_spring(void *void_aux) {
  force_aux_2bodies_t *aux = (force_aux_2bodies_t *)void_aux;
  double k = aux->Constant;
  body_t *body1 = aux->body1;
  body_t *body2 = aux->body2;

  vector_t body1_centroid = body_get_centroid(body1);
  vector_t body2_centroid = body_get_centroid(body2);

  vector_t diff = vec_subtract(body1_centroid, body2_centroid);
  vector_t force_1on2 = vec_multiply(k, diff);

  body_add_force(body1, vec_negate(force_1on2));
  body_add_force(body2, force_1on2);
}

void calc_drag(void *void_aux) {
  force_aux_1body_t *aux = (force_aux_1body_t *)void_aux;
  double gamma = aux->Constant;
  body_t *body = aux->body;

  vector_t force = {-gamma * body_get_velocity(body).x,
                    -gamma * body_get_velocity(body).y};
  body_add_force(body, force);
}

void calc_collision(void *void_aux) {
  force_aux_collision_t *aux = (force_aux_collision_t *)void_aux;
  list_t *shape1 = body_get_shape(aux->body1);
  list_t *shape2 = body_get_shape(aux->body2);
  collision_info_t info = find_collision(shape1, shape2);
  if (!aux->are_colliding && info.collided && aux->body1 != aux->body2) {
    aux->are_colliding = true;
    vector_t axis = info.axis;
    aux->handler(aux->body1, aux->body2, axis, aux->collision_aux);
  } else if (!info.collided) {
    aux->are_colliding = false;
  }
  list_free(shape1);
  list_free(shape2);
}

void calc_destructive_collision(body_t *body1, body_t *body2, vector_t axis,
                                void *void_aux) {
  collision_aux_destructive_t *aux = (collision_aux_destructive_t *)void_aux;
  aux->coll_before_destruct--;

  if (aux->coll_before_destruct == 0) {
    if (aux->body1_is_destroyable) {
      body_remove(body1);
    }
    if (aux->body2_is_destroyable) {
      body_remove(body2);
    }
  }
}

void calc_physics_collision(body_t *body1, body_t *body2, vector_t axis,
                            void *void_aux) {
  collision_aux_physics_t *aux = (collision_aux_physics_t *)void_aux;

  double elasticity = aux->elasticity;
  vector_t v1 = body_get_velocity(body1);
  vector_t v2 = body_get_velocity(body2);
  double m1 = body_get_mass(body1);
  double m2 = body_get_mass(body2);
  double impulse_mag = 0;

  vector_t center_diff =
      vec_subtract(body_get_centroid(body2), body_get_centroid(body1));
  if (vec_dot(center_diff, axis) < 0) {
    axis = vec_negate(axis);
  }

  if (m1 == INFINITY) {
    impulse_mag =
        m2 * (1 + elasticity) * (vec_dot(v2, axis) - vec_dot(v1, axis));
  } else if (m2 == INFINITY) {
    impulse_mag =
        m1 * (1 + elasticity) * (vec_dot(v2, axis) - vec_dot(v1, axis));
  } else {
    impulse_mag = m1 * m2 / (m1 + m2) * (1 + elasticity) *
                  (vec_dot(v2, axis) - vec_dot(v1, axis));
  }
  vector_t impulse_body1 = vec_multiply(impulse_mag, axis);

  body_add_impulse(body1, impulse_body1);
  body_add_impulse(body2, vec_negate(impulse_body1));
}

void calc_normal_force(void *void_aux) {
  force_aux_collision_bodies_t *aux = (force_aux_collision_bodies_t *)void_aux;
  body_t *body1 = aux->body1;
  body_t *body2 = aux->body2;

  list_t *shape1 = body_get_shape(body1);
  list_t *shape2 = body_get_shape(body2);

  collision_info_t collision = find_collision(shape1, shape2);

  vector_t center_diff =
      vec_subtract(body_get_centroid(body2), body_get_centroid(body1));
  if (vec_dot(center_diff, collision.axis) < 0) {
    collision.axis = vec_negate(collision.axis);
  }

  if (!collision.collided) {
    list_free(shape1);
    list_free(shape2);
    return;
  }
  double normal_force_abs_body1 =
      vec_dot(body_get_net_force(body1), collision.axis);
  double normal_force_abs_body2 =
      vec_dot(body_get_net_force(body2), collision.axis);

  if (normal_force_abs_body1 < 0) {
    normal_force_abs_body1 = 0;
  }

  if (normal_force_abs_body2 > 0) {
    normal_force_abs_body2 = 0;
  }

  if (body_get_mass(body2) == INFINITY) {
    vector_t normal =
        vec_negate(vec_multiply(normal_force_abs_body1, collision.axis));
    body_add_force(body1, normal);
  }

  else if (body_get_mass(body1) == INFINITY) {
    vector_t normal =
        vec_negate(vec_multiply(normal_force_abs_body2, collision.axis));
    body_add_force(body2, normal);
  }

  else {
    calc_physics_collision(body1, body2, collision.axis, aux);
  }

  list_free(shape1);
  list_free(shape2);
}

void standard_free_aux(void *aux) { free(aux); }

void free_aux_collision(void *void_aux) {
  force_aux_collision_t *aux = (force_aux_collision_t *)void_aux;
  if (aux->freer != NULL) {
    aux->freer(aux->collision_aux);
  }
  free(aux);
}