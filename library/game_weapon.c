#include "game_weapon.h"
#include "game_const.h"
#include "map.h"
#include "player.h"
#include "sdl_wrapper.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

const double POWERUP_TIME = 40.0;
const double POWERUP_ELASTICITY = 0.15;

const double BULLET_MASS = 1.0;
const double BULLET_LENGTH = 3.0;
const double DEFAULT_BULLET_HEIGHT = 1.0;
const double DEFAULT_BULLET_SPEED = 150.0;

const double SHOT_THRESHOLD = 3.0;
/* --------------------- POWERUPS START ----------------------------
------------------------------------------------------------------*/
vector_t get_random_map1_spawn() {
  list_t *spawns = list_init(4, free);
  // Spawn 1 (center-top)
  vector_t *spawn_point = malloc(sizeof(vector_t));
  *spawn_point = (vector_t){MAX1.x / 2, MAX1.y * 3 / 4};
  list_add(spawns, spawn_point);
  // Spawn 2 (left-mid)
  spawn_point = malloc(sizeof(*spawn_point));
  *spawn_point = (vector_t){MAX1.x / 12, MAX1.y / 2};
  list_add(spawns, spawn_point);
  // Spawn 3 (right-mid)
  spawn_point = malloc(sizeof(*spawn_point));
  *spawn_point = (vector_t){MAX1.x * 11 / 12, MAX1.y / 2};
  list_add(spawns, spawn_point);
  // Spawn 4 (mid-bot)
  spawn_point = malloc(sizeof(*spawn_point));
  *spawn_point = (vector_t){MAX1.x / 2, MAX1.y * 3.7 / 10};
  list_add(spawns, spawn_point);

  vector_t ret = *(vector_t *)list_get(spawns, rand() % 4);
  list_free(spawns);

  return ret;
}

vector_t get_random_map2_spawn() {
  list_t *spawns = list_init(4, free);
  // Spawn 1 (left-top)
  vector_t *spawn_point = malloc(sizeof(vector_t));
  *spawn_point = (vector_t){MAX2.x / 12, MAX2.y * 3.2 / 4};
  list_add(spawns, spawn_point);
  // Spawn 2 (left-mid)
  spawn_point = malloc(sizeof(*spawn_point));
  *spawn_point = (vector_t){MAX2.x / 12, MAX2.y / 2};
  list_add(spawns, spawn_point);
  // Spawn 3 (right-mid)
  spawn_point = malloc(sizeof(*spawn_point));
  *spawn_point = (vector_t){MAX2.x * 11 / 12, MAX2.y / 2};
  list_add(spawns, spawn_point);
  // Spawn 4 (right-top)
  spawn_point = malloc(sizeof(vector_t));
  *spawn_point = (vector_t){MAX2.x * 11.0 / 12, MAX2.y * 3.2 / 4};
  list_add(spawns, spawn_point);

  vector_t ret = *(vector_t *)list_get(spawns, rand() % 4);
  list_free(spawns);

  return ret;
}

body_t *get_powerup(scene_t *scene, body_type_t type) {
  const rgb_color_t POWERUP_RICOCHET_COLOR = {.r = 0.78, .g = 0, .b = 0.98};
  const rgb_color_t POWERUP_SHOTGUN_COLOR = {.r = 0.78, .g = 0, .b = 0.2};
  const double POWERUP_RADIUS = 5.0;
  const double POWERUP_MASS = 1;

  rgb_color_t color = (type == POWERUP_RICOCHET) ? POWERUP_RICOCHET_COLOR
                                                 : POWERUP_SHOTGUN_COLOR;
  body_t *powerup = body_init_with_info(
      rect_init(POWERUP_RADIUS, POWERUP_RADIUS), POWERUP_MASS, color,
      info_init(type, NO_SIDE, NO_WEAPON), free);

  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    if (body_is_removed(body)) {
      continue;
    }
    body_info_t *info = (body_info_t *)body_get_info(body);
    switch (info->type) {
    case PLAYER1:
    case PLAYER2:
      create_powerup_pickup_collision(scene, body, powerup);
      break;
    case WALL:
    case GROUND:
      create_physics_collision(scene, POWERUP_ELASTICITY, powerup, body);
      create_normal_force(scene, powerup, body);
      break;
    case GRAVITY:
      create_newtonian_gravity(scene, G, powerup, body);
      break;
    case POWERUP_RICOCHET:
    case POWERUP_SHOTGUN:
      create_destructive_collision(scene, powerup, body, false, true);
      break;
    default:
      break;
    }
  }

  scene_add_body(scene, powerup);
  return powerup;
}

bool spawn_powerup(scene_t *scene, double time_since_last_drop,
                   double powerups_on_screen, game_state_t map) {
  if (time_since_last_drop < POWERUP_TIME ||
      powerups_on_screen >= MAX_POWERUPS) {
    return false;
  }

  body_type_t powerup_type =
      (rand() % 2 == 0) ? POWERUP_RICOCHET : POWERUP_SHOTGUN;
  body_t *powerup = get_powerup(scene, powerup_type);
  if (map == MAP1) {
    body_set_centroid(powerup, get_random_map1_spawn());
  } else if (map == MAP2 || map == MAP3) {
    body_set_centroid(powerup, get_random_map2_spawn());
  }
  sprite_img_add(scene, powerup, map);

  return true;
}

/* --------------------- BULLET START ------------------------------
------------------------------------------------------------------*/
body_t *bullet_copy(body_t *bullet) {
  list_t *shape_copy = body_get_shape(bullet);
  body_info_t *info_copy = malloc(sizeof(body_info_t));
  *info_copy = *get_info(bullet);
  body_t *copy = body_init_with_info(shape_copy, BULLET_MASS,
                                     body_get_color(bullet), info_copy, free);
  body_set_centroid(copy, body_get_centroid(bullet));
  return copy;
}

body_t *create_pistol_bullet(scene_t *scene, vector_t init_position,
                             side_t dir) {

  const rgb_color_t BULLET_COLOR = {.r = 0.01, .g = 0.98, .b = 0.05};
  list_t *shape = rect_init(BULLET_LENGTH, DEFAULT_BULLET_HEIGHT);

  body_info_t *type = malloc(sizeof(body_info_t));
  *type = (body_info_t){.type = BULLET, .weapon_type = PISTOL};
  rgb_color_t color = BULLET_COLOR;
  vector_t velocity = VEC_ZERO;
  switch (dir) {
  case RIGHT:
    velocity = (vector_t){DEFAULT_BULLET_SPEED, 0.0};
    break;
  case LEFT:
    velocity = (vector_t){-DEFAULT_BULLET_SPEED, 0.0};
    break;
  case UP:
  case DOWN:
  case NO_SIDE:
    break;
  }
  body_t *bullet = body_init_with_info(shape, BULLET_MASS, color, type, free);
  body_set_velocity(bullet, velocity);
  body_set_centroid(bullet, init_position);

  return bullet;
}

body_t *create_ricochet_bullet(scene_t *scene, vector_t init_position,
                               side_t dir) {
  const double RICOCHET_BULLET_HEIGHT = DEFAULT_BULLET_HEIGHT * 2 / 3;
  const rgb_color_t RICOCHET_BULLET_COLOR = {.r = 0.78, .g = 0, .b = 0.98};
  const double RICOCHET_BULLET_SPEED = 1.8 * DEFAULT_BULLET_SPEED;
  const size_t RICOCHET_BULLET_RAND = 120;

  list_t *shape = rect_init(BULLET_LENGTH, RICOCHET_BULLET_HEIGHT);

  body_info_t *type = info_init(BULLET, NO_SIDE, RICOCHET);
  rgb_color_t color = RICOCHET_BULLET_COLOR;
  vector_t velocity = VEC_ZERO;
  switch (dir) {
  case RIGHT:
    velocity =
        (vector_t){RICOCHET_BULLET_SPEED,
                   (rand() % RICOCHET_BULLET_RAND) - RICOCHET_BULLET_RAND / 2};
    break;
  case LEFT:
    velocity =
        (vector_t){-RICOCHET_BULLET_SPEED,
                   (rand() % RICOCHET_BULLET_RAND) - RICOCHET_BULLET_RAND / 2};
    break;
  case UP:
  case DOWN:
  case NO_SIDE:
    break;
  }
  body_t *bullet = body_init_with_info(shape, BULLET_MASS, color, type, free);
  body_set_velocity(bullet, velocity);
  body_set_centroid(bullet, init_position);

  return bullet;
}

body_t *create_first_shotgun_bullet(scene_t *scene, vector_t init_position,
                                    side_t dir) {
  const double SHOTGUN_BULLET_HEIGHT = DEFAULT_BULLET_HEIGHT * 0.35;
  const rgb_color_t SHOTGUN_BULLET_COLOR = {.r = 0.8, .g = 0, .b = 0.18};
  const double SHOTGUN_BULLET_SPEED = 0.6 * DEFAULT_BULLET_SPEED;

  list_t *shape = rect_init(BULLET_LENGTH, SHOTGUN_BULLET_HEIGHT);

  body_info_t *type = info_init(BULLET, NO_SIDE, SHOTGUN);
  rgb_color_t color = SHOTGUN_BULLET_COLOR;
  vector_t velocity = {.x = SHOTGUN_BULLET_SPEED, .y = 0};
  switch (dir) {
  case RIGHT:
    break;
  case LEFT:
    velocity = (vector_t){-velocity.x, velocity.y};
    break;
  case UP:
  case DOWN:
  case NO_SIDE:
    break;
  }
  body_t *bullet = body_init_with_info(shape, BULLET_MASS, color, type, free);
  body_set_velocity(bullet, velocity);
  body_set_centroid(bullet, init_position);
  return bullet;
}

list_t *get_shotgun_bullets(body_t *reference, size_t shots, size_t range,
                            vector_t center) {
  list_t *bullet_list = list_init(shots - 1, (free_func_t)body_free);
  double ref_speed = vec_length(body_get_velocity(reference));

  // used to correct direction if shooting left
  double vel_corr = 1.0;
  if (body_get_centroid(reference).x < center.x) {
    vel_corr = -1.0;
  }

  body_t *new_bullet;
  for (size_t i = 1; i <= shots / 2; i++) {
    new_bullet = bullet_copy(reference);

    double angle = 2.0 * M_PI * i / range;
    body_rotate_about(new_bullet, angle, center);
    vector_t new_vel = {.x = cos(angle) * ref_speed,
                        .y = sin(angle) * ref_speed};
    new_vel = vec_multiply(vel_corr, new_vel);
    body_set_velocity(new_bullet, new_vel);
    list_add(bullet_list, new_bullet);

    new_bullet = bullet_copy(reference);

    body_rotate_about(new_bullet, -angle, center);
    new_vel =
        (vector_t){.x = cos(-angle) * ref_speed, .y = sin(-angle) * ref_speed};
    new_vel = vec_multiply(vel_corr, new_vel);
    body_set_velocity(new_bullet, new_vel);
    list_add(bullet_list, new_bullet);
  }

  return bullet_list;
}

body_t *create_bullet(scene_t *scene, vector_t init_position, side_t dir,
                      game_weapon_type_t type) {
  switch (type) {
  case PISTOL:
    return create_pistol_bullet(scene, init_position, dir);
  case RICOCHET:
    return create_ricochet_bullet(scene, init_position, dir);
  case SHOTGUN:
    return create_first_shotgun_bullet(scene, init_position, dir);
  default:
    break;
  }
  return NULL;
}

void bullet_bind(scene_t *scene, body_t *bullet, game_weapon_type_t weapon_type,
                 body_t *source) {
  const double BULLET_ELASTICITY = 1;
  const double SHOTGUN_RADIUS = 30.0;
  const double RICOCHET_WALL_HITS = 2;

  if (weapon_type == SHOTGUN) {
    create_radial_destructive_collision(scene, source, bullet, false, true,
                                        SHOTGUN_RADIUS);
  }

  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    if (body_is_removed(body)) {
      continue;
    }
    body_info_t *info = (body_info_t *)body_get_info(body);

    switch (info->type) {
    case PLAYER1:
    case PLAYER2: {
      create_destructive_collision(scene, bullet, body, true, true);
      break;
    }
    case BULLET: {
      // Shotgun will destroy any other bullet
      if (weapon_type == SHOTGUN) {
        if (get_info(body)->weapon_type == SHOTGUN) {
          create_physics_collision(scene, BULLET_ELASTICITY, bullet, body);
        } else {
          create_destructive_collision(scene, bullet, body, false, true);
        }
      } else {
        if (get_info(body)->weapon_type == SHOTGUN) {
          create_destructive_collision(scene, bullet, body, true, false);
        } else {
          create_destructive_collision(scene, bullet, body, true, true);
        }
      }
      break;
    }
    case CLOCK_BIG_ARM:
    case CLOCK_SMALL_ARM:
    case WALL:
    case GROUND: {
      switch (weapon_type) {
      case PISTOL:
      case SHOTGUN:
        create_destructive_collision(scene, bullet, body, true, false);
        break;
      case RICOCHET:
        create_physics_collision(scene, BULLET_ELASTICITY, bullet, body);
        create_delayed_destructive_collision(scene, bullet, body, true, false,
                                             RICOCHET_WALL_HITS);
        break;
      default:
        break;
      }
      break;
    }
    case GRAVITY: {
      if (weapon_type != SHOTGUN) {
        create_newtonian_gravity(scene, G, bullet, body);
      }
      break;
    }
    case POWERUP_RICOCHET:
    case POWERUP_SHOTGUN: {
      create_destructive_collision(scene, bullet, body, true, false);
      break;
    }
    default:
      break;
    }
  }
}

/* --------------------- SHOOTING START-----------------------------
------------------------------------------------------------------*/
void game_weapon_upgrade(body_t *player, game_weapon_type_t upgrade) {
  const int SHOTGUN_SHOTS = 2;
  const int RICOCHET_SHOTS = 4;
  body_info_t *info = (body_info_t *)body_get_info(player);
  if (info->weapon_type == upgrade && upgrade != PISTOL) {
    info->shots_left++;
    return;
  }

  info->weapon_type = upgrade;
  switch (info->weapon_type) {
  case SHOTGUN:
    info->shots_left = SHOTGUN_SHOTS;
    break;
  case RICOCHET:
    info->shots_left = RICOCHET_SHOTS;
    break;
  case PISTOL:
    info->shots_left = INFINITY;
    break;
  default:
    break;
  }
}

bool game_weapon_shoot(scene_t *scene, body_t *player) {
  const double BULLET_DISP = 6.0;
  body_info_t *info = (body_info_t *)body_get_info(player);

  if (info->time_since_last_shot < SHOT_THRESHOLD) {
    return false;
  }

  info->time_since_last_shot = 0;
  if (info->shots_left <= 0) {
    game_weapon_upgrade(player, PISTOL);
  }
  if (info->weapon_type != PISTOL) {
    info->shots_left--;
  }

  side_t dir = info->side;
  vector_t disp =
      (dir == LEFT) ? (vector_t){-BULLET_DISP, 0} : (vector_t){BULLET_DISP, 0};
  body_t *bullet = create_bullet(
      scene, vec_add(body_get_centroid(player), disp), dir, info->weapon_type);
  bullet_bind(scene, bullet, info->weapon_type, player);
  scene_add_body(scene, bullet);

  if (info->weapon_type == SHOTGUN) {
    const size_t SHOTGUN_BULLETS = 7;
    const size_t SHOTGUN_SPREAD = SHOTGUN_BULLETS * 12;

    list_t *shotgun_bullet_list = get_shotgun_bullets(
        bullet, SHOTGUN_BULLETS, SHOTGUN_SPREAD, body_get_centroid(player));
    for (size_t i = 0; i < list_size(shotgun_bullet_list); i++) {
      body_t *bullet = (body_t *)list_get(shotgun_bullet_list, i);
      bullet_bind(scene, bullet, get_info(bullet)->weapon_type, player);
      scene_add_body(scene, bullet);
    }

    free(shotgun_bullet_list);
  }

  return true;
}

/* ----------------- COLLISION/FORCE CREATORS ----------------------
------------------------------------------------------------------*/
typedef struct collision_aux_radial {
  body_t *body1;
  body_t *body2;
  bool body1_is_destroyable;
  bool body2_is_destroyable;
  double radius;
} collision_aux_radial_t;

collision_aux_radial_t *collision_aux_radial_init(body_t *body1, body_t *body2,
                                                  bool b1_destroy,
                                                  bool b2_destroy,
                                                  double radius) {
  collision_aux_radial_t *aux = malloc(sizeof(collision_aux_radial_t));
  aux->body1 = body1;
  aux->body2 = body2;
  aux->body1_is_destroyable = b1_destroy;
  aux->body2_is_destroyable = b2_destroy;
  aux->radius = radius;

  return aux;
}

void create_radial_destructive_collision(scene_t *scene, body_t *body1,
                                         body_t *body2, bool body1_destroyable,
                                         bool body2_destroyable,
                                         double radius) {
  collision_aux_radial_t *aux = collision_aux_radial_init(
      body1, body2, body1_destroyable, body2_destroyable, radius);
  list_t *bodies_list = list_init(2, NULL);
  list_add(bodies_list, body1);
  list_add(bodies_list, body2);

  scene_add_bodies_force_creator(scene, calc_radial_destructive_collision, aux,
                                 bodies_list, standard_free_aux);
}

void create_powerup_pickup_collision(scene_t *scene, body_t *player,
                                     body_t *powerup) {
  body_type_t player_type = get_info(player)->type;
  body_type_t powerup_type = get_info(powerup)->type;
  assert((player_type == PLAYER1 || player_type == PLAYER2) &&
         (powerup_type == POWERUP_RICOCHET || powerup_type == POWERUP_SHOTGUN));
  create_collision(scene, player, powerup, calc_pickup_collision,
                   malloc(sizeof(void *)), standard_free_aux);
}

void calc_radial_destructive_collision(void *void_aux) {
  collision_aux_radial_t *aux = (collision_aux_radial_t *)void_aux;
  vector_t b1_pos = body_get_centroid(aux->body1);
  vector_t b2_pos = body_get_centroid(aux->body2);
  double distance = vec_length(vec_subtract(b2_pos, b1_pos));
  if (distance > aux->radius) {
    if (aux->body1_is_destroyable) {
      body_remove(aux->body1);
    }
    if (aux->body2_is_destroyable) {
      body_remove(aux->body2);
    }
  }
}

void calc_pickup_collision(body_t *player, body_t *powerup, vector_t axis,
                           void *void_aux) {
  body_type_t body_type = get_info(powerup)->type;
  if (body_type == POWERUP_RICOCHET) {
    game_weapon_upgrade(player, RICOCHET);
  } else {
    game_weapon_upgrade(player, SHOTGUN);
  }
  body_remove(powerup);
}
