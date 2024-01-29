#include "forces.h"
#include <stdio.h>
#include <stdlib.h>

const size_t gravity_number_of_bodies = 2;
const size_t spring_number_of_bodies = 2;
const size_t drag_number_of_bodies = 1;
const size_t collision_number_of_bodies = 2;

void create_newtonian_gravity(scene_t *scene, double G, body_t *body1,
                              body_t *body2) {

  force_aux_2bodies_t *aux = force_aux_2bodies_init(G, body1, body2);
  list_t *body_targets = list_init(gravity_number_of_bodies, NULL);
  list_add(body_targets, body1);
  list_add(body_targets, body2);

  scene_add_bodies_force_creator(scene, (force_creator_t)calc_gravity, aux,
                                 body_targets, standard_free_aux);
}

void create_normal_force(scene_t *scene, body_t *body1, body_t *body2) {
  force_aux_collision_bodies_t *aux =
      force_aux_collision_bodies_init(body1, body2);
  list_t *body_targets = list_init(collision_number_of_bodies, NULL);
  list_add(body_targets, body1);
  list_add(body_targets, body2);
  scene_add_bodies_force_creator(scene, (force_creator_t)calc_normal_force, aux,
                                 body_targets, standard_free_aux);
}

void create_spring(scene_t *scene, double k, body_t *body1, body_t *body2) {
  force_aux_2bodies_t *aux = force_aux_2bodies_init(k, body1, body2);
  list_t *body_targets = list_init(spring_number_of_bodies, NULL);
  list_add(body_targets, body1);
  list_add(body_targets, body2);

  scene_add_bodies_force_creator(scene, (force_creator_t)calc_spring, aux,
                                 body_targets, standard_free_aux);
}

void create_drag(scene_t *scene, double gamma, body_t *body) {

  force_aux_1body_t *aux = force_aux_1body_init(gamma, body);
  list_t *body_targets = list_init(drag_number_of_bodies, NULL);
  list_add(body_targets, body);

  scene_add_bodies_force_creator(scene, (force_creator_t)calc_drag, aux,
                                 body_targets, standard_free_aux);
}

void create_collision(scene_t *scene, body_t *body1, body_t *body2,
                      collision_handler_t handler, void *aux,
                      free_func_t freer) {
  list_t *body_targets = list_init(collision_number_of_bodies, NULL);
  force_aux_collision_t *collision_aux =
      force_aux_collision_init(body1, body2, handler, aux, freer);
  list_add(body_targets, body1);
  list_add(body_targets, body2);
  scene_add_bodies_force_creator(scene, (force_creator_t)calc_collision,
                                 collision_aux, body_targets,
                                 free_aux_collision);
}

void create_destructive_collision(scene_t *scene, body_t *body1, body_t *body2,
                                  bool body1_is_destroyable,
                                  bool body2_is_destroyable) {
  create_delayed_destructive_collision(
      scene, body1, body2, body1_is_destroyable, body2_is_destroyable, 1);
}

void create_delayed_destructive_collision(
    scene_t *scene, body_t *body1, body_t *body2, bool body1_is_destroyable,
    bool body2_is_destroyable, size_t collisions_before_destruction) {
  collision_aux_destructive_t *aux =
      collision_aux_destructive_init(body1_is_destroyable, body2_is_destroyable,
                                     collisions_before_destruction);
  create_collision(scene, body1, body2,
                   (collision_handler_t)calc_destructive_collision, aux,
                   standard_free_aux);
}

void create_physics_collision(scene_t *scene, double elasticity, body_t *body1,
                              body_t *body2) {
  collision_aux_physics_t *aux = collision_aux_physics_init(elasticity);
  create_collision(scene, body1, body2,
                   (collision_handler_t)calc_physics_collision, aux,
                   standard_free_aux);
}
