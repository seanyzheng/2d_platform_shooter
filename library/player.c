#include "player.h"
#include "game_const.h"

const double PLAYER_WIDTH = 6.0;
const double PLAYER_HEIGHT = 9.0;
const vector_t START_VELOCITY = {.x = 0.0, .y = 15.0};
const rgb_color_t PLAYER_FEET_COLOR = {.r = 0, .g = 0, .b = 0};
const double PLAYER_FEET_HEIGHT = 1.0;
const double PLAYER_DRAG = 0.5;
const double WALL_ELASTICITY = 0.5;
const double GROUND_ELASTICITY = 0.0;

body_info_t *info_init(body_type_t type, side_t side,
                       game_weapon_type_t weapon) {
  body_info_t *info = malloc(sizeof(body_info_t));
  info->type = type;
  info->side = side;
  info->weapon_type = weapon;
  info->time_since_last_shot = 0;
  info->shots_left = INFINITY;
  return info;
}

body_info_t *get_info(body_t *body) {
  return (body_info_t *)body_get_info(body);
}

body_t *get_player(vector_t center, vector_t velocity, body_type_t type,
                   side_t dir) {
  list_t *shape = rect_init(PLAYER_WIDTH, PLAYER_HEIGHT);
  rgb_color_t color = type == PLAYER1 ? PLAYER_1_COLOR : PLAYER_2_COLOR;
  body_t *player = body_init_with_info(shape, PLAYER_MASS, color,
                                       info_init(type, dir, PISTOL), free);

  body_set_centroid(player, center);

  return player;
}

/** Creates a player with the given starting position and velocity */
body_t *add_player(scene_t *scene, body_type_t type, vector_t spawn) {
  // Add the player to the scene.
  body_t *player;
  if (type == PLAYER1) {
    player = get_player(spawn, START_VELOCITY, type, RIGHT);
  } else {
    player = get_player(spawn, START_VELOCITY, type, LEFT);
  }

  create_drag(scene, PLAYER_DRAG, player);
  size_t body_count = scene_bodies(scene);
  scene_add_body(scene, player);

  // Add force creators with other bodies
  for (size_t i = 0; i < body_count; i++) {
    body_t *body = scene_get_body(scene, i);
    switch (get_info(body)->type) {
    case PLAYER1:
    case PLAYER2:
    // Prevent duplicate force creators
    case BULLET:
    case POWERUP_RICOCHET:
    case POWERUP_SHOTGUN:
      break;
    case WALL:
      create_physics_collision(scene, WALL_ELASTICITY, player, body);
      create_normal_force(scene, player, body);
      break;
    case GROUND:
      create_physics_collision(scene, GROUND_ELASTICITY, player, body);
      create_normal_force(scene, player, body);
      break;
    case GRAVITY:
      create_newtonian_gravity(scene, G, body, player);
      break;
    default:
      break;
    }
  }

  return player;
}

body_t *get_player_feet(body_t *player) {
  list_t *shape = rect_init(PLAYER_WIDTH, PLAYER_FEET_HEIGHT);
  body_t *player_feet = body_init(shape, PLAYER_MASS, PLAYER_FEET_COLOR);
  vector_t centroid = body_get_centroid(player);
  body_set_centroid(player_feet,
                    (vector_t){centroid.x, centroid.y - PLAYER_HEIGHT / 2});

  return player_feet;
}

/** Returns pointer to specified player */
body_t *fetch_object(scene_t *scene, body_type_t body_type) {
  body_t *obj = NULL;
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    if (get_info(scene_get_body(scene, i))->type == body_type) {
      obj = scene_get_body(scene, i);
      break;
    }
  }
  return obj;
}

/** Returns pointer to specified body_type
 * Undefined behavior when there are multiple bodies of the same type in scene
 */
sprite_t *fetch_sprite(scene_t *scene, body_type_t body_type) {
  sprite_t *obj = NULL;
  size_t sprites_size = list_size(scene_get_sprites(scene));
  for (size_t i = 0; i < sprites_size; i++) {
    sprite_t *sprite = scene_get_sprite(scene, i);
    if (get_info(sprite_get_body(sprite))->type == body_type) {
      obj = sprite;
      break;
    }
  }
  return obj;
}