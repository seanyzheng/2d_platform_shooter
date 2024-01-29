#include "scene.h"
#include "game.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

const size_t INITIAL_CAPACITY_S = 20;

// FORCE BIND DEFINITION AND FUNCTIONS
typedef struct force_bind {
  force_creator_t force_function;
  void *aux;
  list_t *body_targets;
  free_func_t freer;
} force_bind_t;

void force_bind_free(force_bind_t *force_bind) {
  if (force_bind->freer != NULL) {
    force_bind->freer(force_bind->aux);
  }
  if (force_bind->body_targets != NULL) {
    list_free(force_bind->body_targets);
  }
  free(force_bind);
}

bool bind_is_removed(force_bind_t *force_bind) {
  if (force_bind->body_targets == NULL) {
    return false;
  }
  for (size_t j = 0; j < list_size(force_bind->body_targets); j++) {
    body_t *body = list_get(force_bind->body_targets, j);
    if (body_is_removed(body)) {
      return true;
    }
  }

  return false;
}
// END OF FORCE_BIND DEFINITION

typedef struct scene {
  list_t *bodies;
  list_t *force_binds;
  list_t *list_of_sprites;
} scene_t;

scene_t *scene_init(void) {
  scene_t *scene = malloc(sizeof(scene_t));
  *scene =
      (scene_t){.bodies = list_init(INITIAL_CAPACITY_S, (free_func_t)body_free),
                .force_binds =
                    list_init(INITIAL_CAPACITY_S, (free_func_t)force_bind_free),
                .list_of_sprites =
                    list_init(INITIAL_CAPACITY_S, (free_func_t)sprite_free)};
  assert(scene != NULL);

  return scene;
}

void sprite_list_init(scene_t *scene) {
  size_t body_count = scene_bodies(scene);
  for (size_t i = 0; i < body_count; i++) {
    body_t *body = scene_get_body(scene, i);
    if (((body_info_t *)body_get_info(body))->type != GRAVITY) {
      sprite_t *new_sprite = sprite_init(body);
      scene_add_sprite(scene, new_sprite);
    }
  }
}

void sprite_list_update(scene_t *scene) {
  size_t sprite_count = list_size(scene->list_of_sprites);
  for (size_t i = 0; i < sprite_count; i++) {
    sprite_t *sprite = scene_get_sprite(scene, i);
    sprite_update(sprite);
  }
}

void scene_free(scene_t *scene) {
  list_free(scene->bodies);
  list_free(scene->force_binds);
  list_free(scene->list_of_sprites);
  free(scene);
}

size_t scene_bodies(scene_t *scene) { return list_size(scene->bodies); }

body_t *scene_get_body(scene_t *scene, size_t index) {
  assert(index < list_size(scene->bodies));
  return (body_t *)(list_get(scene->bodies, index));
}

void scene_add_body(scene_t *scene, body_t *body) {
  list_add(scene->bodies, body);
}

void scene_remove_body(scene_t *scene, size_t index) {
  body_remove(list_get(scene->bodies, index));
}
void scene_add_sprite(scene_t *scene, sprite_t *sprite) {
  list_add(scene->list_of_sprites, sprite);
}

void scene_remove_sprite(scene_t *scene, size_t index) {
  sprite_free((sprite_t *)list_remove(scene->list_of_sprites, index));
}

list_t *scene_get_sprites(scene_t *scene) { return scene->list_of_sprites; }

sprite_t *scene_get_sprite(scene_t *scene, size_t index) {
  assert(index < list_size(scene->list_of_sprites));
  return (sprite_t *)(list_get(scene->list_of_sprites, index));
}

void scene_add_bodies_force_creator(scene_t *scene, force_creator_t forcer,
                                    void *aux, list_t *bodies,
                                    free_func_t freer) {
  force_bind_t *force_bind = malloc(sizeof(force_bind_t));
  force_bind->aux = aux;
  force_bind->body_targets = bodies;
  force_bind->freer = freer;
  force_bind->force_function = forcer;
  list_add(scene->force_binds, force_bind);
}

void scene_add_force_creator(scene_t *scene, force_creator_t forcer, void *aux,
                             free_func_t freer) {
  scene_add_bodies_force_creator(scene, forcer, aux, NULL, freer);
}

void scene_tick(scene_t *scene, double dt) {

  // Execute all forces in scene
  for (size_t i = 0; i < list_size(scene->force_binds); i++) {
    force_bind_t *force_bind = (force_bind_t *)list_get(scene->force_binds, i);
    force_bind->force_function(force_bind->aux);
  }

  // Remove force binds if body is_removed == true
  for (int i = 0; i < list_size(scene->force_binds); i++) {
    force_bind_t *force_bind = (force_bind_t *)list_get(scene->force_binds, i);
    if (bind_is_removed(force_bind)) {
      force_bind_free(list_remove(scene->force_binds, i));
      i -= 1; // fix current index after removal of item from list
    }
  }

  // Remove bodies where is_removed == true and have a sprite
  for (int i = 0; i < list_size(scene->list_of_sprites); i++) {
    sprite_t *sprite = list_get(scene->list_of_sprites, i);
    if (sprite_is_removed(sprite)) {
      sprite_free(list_remove(scene->list_of_sprites, i));
      i -= 1;
    }
  }

  // Remove bodies where is_removed == true
  for (int i = 0; i < list_size(scene->bodies); i++) {
    body_t *body = (body_t *)list_get(scene->bodies, i);

    if (body_is_removed(body)) {
      body_free(list_remove(scene->bodies, i));
      i -= 1; // fix current index after removal of item from list
      continue;
    }

    body_tick(body, dt);
  }
}