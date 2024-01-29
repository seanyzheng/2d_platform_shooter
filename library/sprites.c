#include "sprites.h"
#include "body.h"
#include "game.h"
#include "list.h"
#include "vector.h"
#include <assert.h>

typedef struct sprite {
  char *path;
  list_t *tex;
  SDL_Rect *destR;
  body_t *body;
  size_t tex_index;
} sprite_t;

sprite_t *sprite_init(body_t *body) {
  size_t TEXT_INITIAL_CAPACITY = 4;

  sprite_t *new_sprite = malloc(sizeof(sprite_t));
  SDL_Rect *destR = malloc(sizeof(SDL_Rect));
  vector_t window_center = get_window_center();

  vector_t *top_right = list_get(body_get_shape(body), 3);
  vector_t *bottom_left = list_get(body_get_shape(body), 1);
  vector_t *top_left = list_get(body_get_shape(body), 0);

  vector_t top_left_pix = get_window_position(*top_left, window_center);
  vector_t top_right_pix = get_window_position(*top_right, window_center);
  vector_t bottom_left_pix = get_window_position(*bottom_left, window_center);

  destR->x = top_left_pix.x;
  destR->y = top_left_pix.y;
  destR->w = top_right_pix.x - bottom_left_pix.x;
  destR->h = bottom_left_pix.y - top_right_pix.y;

  new_sprite->destR = destR;
  new_sprite->body = body;
  new_sprite->path = malloc(sizeof(char));
  free(top_right);
  free(bottom_left);
  free(top_left);

  new_sprite->tex = list_init(TEXT_INITIAL_CAPACITY, (free_func_t)free);
  new_sprite->tex_index = 0;
  return new_sprite;
}

// updates texture and surface based on body type
void sprite_update(sprite_t *sprite) {
  sprite_t *new_sprite = sprite_init(sprite_get_body(sprite));

  sprite->destR = new_sprite->destR;
  free(new_sprite);
}

body_t *sprite_get_body(sprite_t *sprite) { return sprite->body; }

void sprite_add_tex(sprite_t *sprite, SDL_Texture *tex) {
  list_add(sprite->tex, tex);
}

SDL_Texture *sprite_get_tex(sprite_t *sprite, size_t index) {
  return list_get(sprite->tex, index);
}

void sprite_set_tex(sprite_t *sprite, size_t index) {
  sprite->tex_index = index;
}

size_t sprite_textures(sprite_t *sprite) { return list_size(sprite->tex); }

SDL_Rect *sprite_get_destR(sprite_t *sprite) { return sprite->destR; }

void sprite_set_destR(sprite_t *sprite, SDL_Rect destR) {
  *(sprite->destR) = destR;
}

size_t sprite_get_curr_ind(sprite_t *sprite) { return sprite->tex_index; }

bool sprite_is_removed(sprite_t *sprite) {
  assert(sprite->body != NULL);
  return body_is_removed(sprite->body);
}

void sprite_free(sprite_t *sprite) {
  free(sprite->tex);
  free(sprite->path);
  free(sprite->destR);
  free(sprite);
}
