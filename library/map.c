#include "map.h"
#include "game_const.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

const double WALL_WIDTH = 4.0;
const rgb_color_t WALL_COLOR = {.r = 0, .g = 0, .b = 1};
const rgb_color_t BACKGROUND_COLOR = {.r = 0, .g = 1, .b = 0};
const size_t CIRCLE_POINTS = 40;
const double GRAVITY_M = 6E24;    // kg
const double GRAVITY_CONST = 150; // m / s^2

void add_gravity_body(scene_t *scene) {
  double GRAVITY_R = (sqrt(G * GRAVITY_M / GRAVITY_CONST));

  list_t *gravity_player = rect_init(1, 1);
  body_t *body =
      body_init_with_info(gravity_player, GRAVITY_M, WALL_COLOR,
                          info_init(GRAVITY, NO_SIDE, NO_WEAPON), free);

  // Move a distance R below the scene
  vector_t gravity_center = {.x = MAX1.x / 2, .y = -GRAVITY_R};
  body_set_centroid(body, gravity_center);
  scene_add_body(scene, body);
}

// Menu
void generate_menu(scene_t *scene) {
  list_t *rect = rect_init(MAX_MENU.x, MAX_MENU.y);
  body_t *body =
      body_init_with_info(rect, INFINITY, BACKGROUND_COLOR,
                          info_init(BACKGROUND, NO_SIDE, NO_WEAPON), free);
  body_set_centroid(body, (vector_t){.x = MAX_MENU.x / 2, .y = MAX_MENU.y / 2});
  scene_add_body(scene, body);
}

void add_platform(scene_t *scene, double width, double height, double mass,
                  vector_t position, rgb_color_t color,
                  body_info_t *body_info) {
  list_t *rect = rect_init(width, height);
  body_t *body = body_init_with_info(rect, mass, color, body_info, free);
  body_set_centroid(body, position);
  scene_add_body(scene, body);
}

// Map 1
void generate_map1(scene_t *scene) {

  float PLATFORM_LENGTH = 10.0;
  float PLATFORM_WIDTH = 3.0;
  float PLATFORM1_X = 5.0;
  float PLATFORM1_Y = MAX1.y / 4.0;
  float PLATFORM2_X = MAX1.x - 5.0;
  float PLATFORM2_Y = MAX1.y / 4.0;
  float SEPARATING_WALL_HEIGHT = 18.0;
  float SEPARATING_WALL_WIDTH = 3.0;
  float SEPARATING_WALL_X = MAX1.x / 2.0;
  float SEPARATING_WALL_Y = 11.5;
  float PLATFORM3_LENGTH = MAX1.x / 2.0;
  float PLATFORM3_X = MAX1.x / 2.0;
  float PLATFORM3_Y = MAX1.y / 2.5;
  rgb_color_t PLATFORM_COLOR = ((rgb_color_t){0, 0, 1});

  // Background
  add_platform(scene, MAX1.x, MAX1.y, INFINITY,
               (vector_t){.x = MAX1.x / 2, .y = MAX1.y / 2}, BACKGROUND_COLOR,
               info_init(BACKGROUND, NO_SIDE, NO_WEAPON));

  // Left wall]
  add_platform(scene, WALL_WIDTH, 2 * MAX1.y, INFINITY,
               (vector_t){.x = 0.0, .y = 0.0}, WALL_COLOR,
               info_init(WALL, NO_SIDE, NO_WEAPON));

  // Left platform
  add_platform(scene, PLATFORM_LENGTH, PLATFORM_WIDTH, INFINITY,
               (vector_t){.x = PLATFORM1_X, .y = PLATFORM1_Y}, PLATFORM_COLOR,
               info_init(GROUND, NO_SIDE, NO_WEAPON));

  // Right wall

  add_platform(scene, WALL_WIDTH, 2 * MAX1.y, INFINITY,
               (vector_t){.x = MAX1.x, .y = 0.0}, WALL_COLOR,
               info_init(WALL, NO_SIDE, NO_WEAPON));

  // Right platform
  add_platform(scene, PLATFORM_LENGTH, PLATFORM_WIDTH, INFINITY,
               (vector_t){.x = PLATFORM2_X, .y = PLATFORM2_Y}, PLATFORM_COLOR,
               info_init(GROUND, NO_SIDE, NO_WEAPON));

  // Ground (left)
  add_platform(scene, MAX1.x / 2, 2 * PLATFORM_WIDTH, INFINITY,
               (vector_t){.x = MAX1.x / 4, .y = PLATFORM_WIDTH}, PLATFORM_COLOR,
               info_init(GROUND, NO_SIDE, NO_WEAPON));

  // Ground (right)
  add_platform(scene, MAX1.x / 2, 2 * PLATFORM_WIDTH, INFINITY,
               (vector_t){.x = 3 * MAX1.x / 4, .y = PLATFORM_WIDTH},
               PLATFORM_COLOR, info_init(GROUND, NO_SIDE, NO_WEAPON));

  // Separating wall
  add_platform(scene, SEPARATING_WALL_WIDTH, SEPARATING_WALL_HEIGHT, INFINITY,
               (vector_t){.x = SEPARATING_WALL_X, .y = SEPARATING_WALL_Y},
               WALL_COLOR, info_init(WALL, NO_SIDE, NO_WEAPON));

  // Ceiling
  add_platform(scene, MAX1.x, WALL_WIDTH, INFINITY,
               (vector_t){.x = MAX1.x / 2.0, .y = MAX1.y}, WALL_COLOR,
               info_init(WALL, NO_SIDE, NO_WEAPON));

  // Central platform
  add_platform(scene, PLATFORM3_LENGTH, 1.5 * WALL_WIDTH, INFINITY,
               (vector_t){.x = PLATFORM3_X, .y = PLATFORM3_Y}, WALL_COLOR,
               info_init(GROUND, NO_SIDE, NO_WEAPON));
}

/* -------------------- Map 2 ---------------------------------------
-------------------------------------------------------------------*/
void generate_map2(scene_t *scene) {

  float GROUND_WIDTH = 8.0;

  rgb_color_t PLATFORM_COLOR = ((rgb_color_t){0, 0, 1});
  float PLATFORM_LENGTH = 40.0;
  float PLATFORM_WIDTH = 3.0;
  float PLATFORM1_X = 5.0;
  float PLATFORM1_Y = MAX2.y / 3.0;
  float PLATFORM2_X = MAX2.x - 5.0;
  float PLATFORM2_Y = MAX2.y / 3.0;

  rgb_color_t TELEPORTER_COLOR = ((rgb_color_t){0, 0, 0});

  rgb_color_t CLOCK_BACKGROUND_COLOR = ((rgb_color_t){0.9647, 0.8157, 0.6941});
  float TELEPORTER_LENGTH = 10.0;
  float TELEPORTER_WIDTH = 3.0;
  float TELEPORTER1_X = MAX2.x / 5.0;
  float TELEPORTER1_Y = MAX2.y / 6.0;
  float TELEPORTER2_X = 4 * MAX2.x / 5.0;
  float TELEPORTER2_Y = MAX2.y / 6.0;

  // Background
  add_platform(scene, MAX2.x, MAX2.y, INFINITY,
               (vector_t){.x = MAX2.x / 2, .y = MAX2.y / 2}, BACKGROUND_COLOR,
               info_init(BACKGROUND, NO_SIDE, NO_WEAPON));

  // Ground (left)
  add_platform(scene, 2 * MAX2.x / 3.0, GROUND_WIDTH, INFINITY,
               (vector_t){.x = 0.0, .y = GROUND_WIDTH / 25}, WALL_COLOR,
               info_init(GROUND, NO_SIDE, NO_WEAPON));

  // Ground (right)
  add_platform(scene, 2 * MAX2.x / 3.0, GROUND_WIDTH, INFINITY,
               (vector_t){.x = MAX2.x, .y = GROUND_WIDTH / 25}, WALL_COLOR,
               info_init(GROUND, NO_SIDE, NO_WEAPON));

  // Ceiling (left)
  add_platform(scene, 2 * MAX2.x / 3.0, WALL_WIDTH, INFINITY,
               (vector_t){.x = MAX2.x / 6.0, .y = MAX2.y}, WALL_COLOR,
               info_init(WALL, NO_SIDE, NO_WEAPON));

  // Ceiling (right)
  add_platform(scene, 2 * MAX2.x / 3.0, WALL_WIDTH, INFINITY,
               (vector_t){.x = MAX2.x * 5 / 6.0, .y = MAX2.y}, WALL_COLOR,
               info_init(WALL, NO_SIDE, NO_WEAPON));

  // Clock Background
  list_t *rect = circle_init(MAX2.x / 5.5, CIRCLE_POINTS);
  body_t *body =
      body_init_with_info(rect, INFINITY, ((rgb_color_t){0.0, 0.0, 0.0}),
                          info_init(CLOCK, NO_SIDE, NO_WEAPON), free);
  body_set_centroid(body, (vector_t){.x = MAX2.x / 2.0, .y = MAX2.y / 2.0});
  scene_add_body(scene, body);
  rect = circle_init(MAX2.x / 5.7, CIRCLE_POINTS);
  body = body_init_with_info(rect, INFINITY, CLOCK_BACKGROUND_COLOR,
                             info_init(CLOCK, NO_SIDE, NO_WEAPON), free);
  body_set_centroid(body, (vector_t){.x = MAX2.x / 2.0, .y = MAX2.y / 2.0});
  scene_add_body(scene, body);

  // Clock big arm
  rect = rect_init(MAX2.x / 6.0, WALL_WIDTH);
  body =
      body_init_with_info(rect, INFINITY, ((rgb_color_t){0, 0, 0}),
                          info_init(CLOCK_BIG_ARM, NO_SIDE, NO_WEAPON), free);
  body_set_centroid(
      body, (vector_t){.x = MAX2.x / 2.0 - MAX2.x / 12.0, .y = MAX2.y / 2.0});
  body_set_rot_velocity(body, 0.001);
  body_set_rot_acceleration(body, 0.0008);
  body_set_rotation_center(body, (vector_t){.x = MAX2.x / 2, .y = MAX2.y / 2});
  scene_add_body(scene, body);

  // Clock small arm
  rect = rect_init(MAX2.x / 8.0, WALL_WIDTH / 2);
  body =
      body_init_with_info(rect, INFINITY, ((rgb_color_t){1, 0, 0}),
                          info_init(CLOCK_SMALL_ARM, NO_SIDE, NO_WEAPON), free);
  body_set_centroid(
      body, (vector_t){.x = MAX2.x / 2.0 - MAX2.x / 16, .y = MAX2.y / 2.0});
  body_set_rot_velocity(body, 0.01);
  body_set_rot_acceleration(body, 0.001);
  body_set_rotation_center(body, (vector_t){.x = MAX2.x / 2, .y = MAX2.y / 2});
  scene_add_body(scene, body);

  // Right platforms
  add_platform(scene, PLATFORM_LENGTH, PLATFORM_WIDTH, INFINITY,
               (vector_t){.x = PLATFORM2_X, .y = PLATFORM2_Y}, PLATFORM_COLOR,
               info_init(GROUND, NO_SIDE, NO_WEAPON));

  add_platform(scene, PLATFORM_LENGTH, PLATFORM_WIDTH, INFINITY,
               (vector_t){.x = PLATFORM2_X, .y = PLATFORM2_Y + MAX2.y / 3},
               PLATFORM_COLOR, info_init(GROUND, NO_SIDE, NO_WEAPON));

  // Left platforms
  add_platform(scene, PLATFORM_LENGTH, PLATFORM_WIDTH, INFINITY,
               (vector_t){.x = PLATFORM1_X, .y = PLATFORM1_Y}, PLATFORM_COLOR,
               info_init(GROUND, NO_SIDE, NO_WEAPON));

  add_platform(scene, PLATFORM_LENGTH, PLATFORM_WIDTH, INFINITY,
               (vector_t){.x = PLATFORM1_X, .y = PLATFORM1_Y + MAX2.y / 3},
               PLATFORM_COLOR, info_init(GROUND, NO_SIDE, NO_WEAPON));

  // Teleporter Platforms Left
  add_platform(scene, TELEPORTER_LENGTH, TELEPORTER_WIDTH, INFINITY,
               (vector_t){.x = TELEPORTER1_X, .y = TELEPORTER1_Y},
               TELEPORTER_COLOR, info_init(GROUND, NO_SIDE, NO_WEAPON));

  add_platform(scene, TELEPORTER_LENGTH, TELEPORTER_WIDTH, INFINITY,
               (vector_t){.x = TELEPORTER1_X, .y = MAX2.y / 2},
               TELEPORTER_COLOR, info_init(GROUND, NO_SIDE, NO_WEAPON));

  add_platform(scene, TELEPORTER_LENGTH, TELEPORTER_WIDTH, INFINITY,
               (vector_t){.x = TELEPORTER1_X, .y = MAX2.y - TELEPORTER1_Y},
               TELEPORTER_COLOR, info_init(GROUND, NO_SIDE, NO_WEAPON));

  // Teleporter Platforms Right
  add_platform(scene, TELEPORTER_LENGTH, TELEPORTER_WIDTH, INFINITY,
               (vector_t){.x = TELEPORTER2_X, .y = TELEPORTER2_Y},
               TELEPORTER_COLOR, info_init(GROUND, NO_SIDE, NO_WEAPON));

  add_platform(scene, TELEPORTER_LENGTH, TELEPORTER_WIDTH, INFINITY,
               (vector_t){.x = TELEPORTER2_X, .y = MAX2.y / 2},
               TELEPORTER_COLOR, info_init(GROUND, NO_SIDE, NO_WEAPON));

  add_platform(scene, TELEPORTER_LENGTH, TELEPORTER_WIDTH, INFINITY,
               (vector_t){.x = TELEPORTER2_X, .y = MAX2.y - TELEPORTER2_Y},
               TELEPORTER_COLOR, info_init(GROUND, NO_SIDE, NO_WEAPON));
}

//-----------------------------------------------------

void create_map(scene_t *scene, game_state_t game_state) {
  if (game_state == INTRO_MENU || game_state == MAIN_MENU ||
      game_state == LORE || game_state == MAP_SELECT || game_state == CREDITS ||
      game_state == INSTRUCTIONS || game_state == GAME_WIN_P1 ||
      game_state == GAME_WIN_P2) {
    generate_menu(scene);
  }

  if (game_state == MAP1 || game_state == MAP2 || game_state == MAP3) {
    vector_t MAP1_P1_SPAWN = ((vector_t){.x = MAX1.x / 4, .y = 12});
    vector_t MAP1_P2_SPAWN = ((vector_t){.x = 3 * MAX1.x / 4, .y = 12});

    vector_t MAP2_P1_SPAWN = ((vector_t){.x = MAX2.x / 4, .y = 12});
    vector_t MAP2_P2_SPAWN = ((vector_t){.x = 3 * MAX2.x / 4, .y = 12});

    list_t *players_list = list_init(2, (free_func_t)body_free);

    add_gravity_body(scene);

    if (game_state == MAP1) {
      generate_map1(scene);
      list_add(players_list, add_player(scene, PLAYER1, MAP1_P1_SPAWN));
      list_add(players_list, add_player(scene, PLAYER2, MAP1_P2_SPAWN));
    }

    if (game_state == MAP2 || game_state == MAP3) {
      generate_map2(scene);
      list_add(players_list, add_player(scene, PLAYER1, MAP2_P1_SPAWN));
      list_add(players_list, add_player(scene, PLAYER2, MAP2_P2_SPAWN));
    }
  }
}

void check_bounds(body_t *body) {
  double TOLERANCE = 6.2;
  vector_t position = body_get_centroid(body);
  if (position.x < (-TOLERANCE)) {
    body_set_centroid(body,
                      (vector_t){.x = MAX2.x - TOLERANCE, .y = position.y});
  }

  else if (position.x > (MAX2.x + TOLERANCE)) {
    body_set_centroid(body, (vector_t){.x = TOLERANCE, .y = position.y});
  }

  else if (position.y < (-TOLERANCE)) {
    body_set_centroid(body,
                      (vector_t){.x = position.x, .y = MAX2.y - TOLERANCE});
  }

  else if (position.y > (MAX2.y + TOLERANCE)) {
    body_set_centroid(body, (vector_t){.x = position.x, .y = TOLERANCE});
  }
}
