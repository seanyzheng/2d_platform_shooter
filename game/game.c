#include "game_const.h"
#include "game_weapon.h"
#include "map.h"
#include "scene.h"
#include "sdl_wrapper.h"
#include "vector.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct state {
  scene_t *scene;
  bool *key_states;
  list_t *sound_effects;
  game_state_t game_state;
  double time_since_drop;
  double time_since_respawn;
  double time_since_p1_jump;
  double time_since_p2_jump;
  size_t p1lives;
  size_t p2lives;
  bool story_mode;
} state_t;

const double TIME_THRESHOLD = 1.0;
const double TIME_MULT = 10.0;

const size_t NUM_OF_KEYS = 10;

const double ANGLE_ERROR = 0.1;
const double ANGULAR_MULTIPLIER_BIG = 1.3;
const double ANGULAR_MULTIPLIER_SMALL = 1.5;

// Player
const double PLAYER_ACCELERATION = 5000.0;
const double PLAYER_MAX_SPEED = 40.0;
const int STARTING_LIVES = 5;
const double LIVES_WIDTH = 5.0;
const double LIVES_HEIGHT = 5.0;

/** Generates a random number between 0 and 1 */
double rand_double(void) { return (double)rand() / RAND_MAX; }

bool in_game(state_t *state) {
  game_state_t game_state = state->game_state;
  if (game_state == MAP1 || game_state == MAP2 || game_state == MAP3) {
    return true;
  }

  return false;
}

body_t *get_life(vector_t center, body_type_t type) {
  list_t *shape = rect_init(LIVES_WIDTH, LIVES_HEIGHT);
  rgb_color_t color = type == P1_LIFE ? PLAYER_1_COLOR : PLAYER_2_COLOR;
  body_t *life = body_init_with_info(shape, 1, color,
                                     info_init(type, NO_SIDE, NO_WEAPON), free);
  body_set_centroid(life, center);

  return life;
}

void add_lives(state_t *state) {
  double LIVES_SPACING = 4.5;

  if (state->game_state != MAP1 && state->game_state != MAP2 &&
      state->game_state != MAP3) {
    return;
  }

  vector_t Max = (vector_t){VEC_ZERO.x, VEC_ZERO.y};
  if (state->game_state == MAP1) {
    Max = (vector_t){MAX1.x, MAX1.y};
  } else if (state->game_state == MAP2 || state->game_state == MAP3) {
    Max = (vector_t){MAX2.x, MAX2.y};
  }
  for (size_t i = 0; i < state->p1lives; i++) {
    body_t *life = get_life((vector_t){(LIVES_SPACING + 1 / 2 * LIVES_WIDTH) +
                                           (LIVES_SPACING + LIVES_WIDTH) * i,
                                       Max.y - LIVES_SPACING},
                            P1_LIFE);
    scene_add_body(state->scene, life);
    sprite_img_add(state->scene, life, state->game_state);
  }

  for (size_t i = 0; i < state->p2lives; i++) {
    body_t *life =
        get_life((vector_t){Max.x - ((LIVES_SPACING + 1 / 2 * LIVES_WIDTH) +
                                     (LIVES_SPACING + LIVES_WIDTH) * i),
                            Max.y - LIVES_SPACING},
                 P2_LIFE);
    scene_add_body(state->scene, life);
    sprite_img_add(state->scene, life, state->game_state);
  }
}

// ---------------------- KEY EVENTS
// ---------------------------------------------------------------------
void jumping_handler(state_t *state, body_t *player) {
  vector_t PLAYER_JUMP = {.x = 0.0, .y = (PLAYER_MASS * 85)};

  body_t *player_feet = get_player_feet(player);
  scene_t *scene = state->scene;

  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_info_t *info = get_info(scene_get_body(scene, i));
    if (info->type == GROUND) {
      if (find_collision(body_get_shape(player_feet),
                         body_get_shape(scene_get_body(scene, i)))
              .collided) {
        sdl_sound_effects(state, JUMP);
        body_add_impulse(player, PLAYER_JUMP);
        break;
      }
    }
  }
  body_free(player_feet);
}

void player_shoot(state_t *state, body_t *player) {
  scene_t *scene = state->scene;
  bool shot = game_weapon_shoot(scene, player);
  if (shot) {
    switch (get_info(player)->weapon_type) {
    case PISTOL: {
      sdl_sound_effects(state, PISTOL_S);
      break;
    }
    case SHOTGUN: {
      sdl_sound_effects(state, SHOTGUN_S);
      break;
    }
    case RICOCHET: {
      sdl_sound_effects(state, RICOCHET_S);
      break;
    }
    default:
      break;
    }
  }
}

void apply_key_states(state_t *state, body_t *player1, body_t *player2) {
  if (state->key_states[A_KEY] == false && state->key_states[D_KEY] == false) {
    body_set_velocity(player1, (vector_t){0.0, body_get_velocity(player1).y});
  }
  if (state->key_states[LEFT_ARROW] == false &&
      state->key_states[RIGHT_ARROW] == false) {
    body_set_velocity(player2, (vector_t){0.0, body_get_velocity(player2).y});
  }

  if (state->key_states[SPACE]) {
    player_shoot(state, player1);
  }
  if (state->key_states[PERIOD]) {
    player_shoot(state, player2);
  }

  if (state->key_states[W_KEY]) {
    if (state->time_since_p1_jump > TIME_THRESHOLD) {
      jumping_handler(state, player1);
      state->time_since_p1_jump = 0;
    }
  }
  if (state->key_states[UP_ARROW]) {
    if (state->time_since_p2_jump > TIME_THRESHOLD) {
      jumping_handler(state, player2);
      state->time_since_p2_jump = 0;
    }
  }
}

void menu_handler(state_t *state, game_state_t new_game_state) {
  sdl_sound_effects(state, CLICK);
  scene_free(state->scene);
  state->game_state = new_game_state;
  state->scene = scene_init();
  if (new_game_state == MAP2 || new_game_state == MAP3) {
    sdl_init((vector_t)VEC_ZERO, (vector_t)MAX2);
  } else if (new_game_state == MAP1) {
    sdl_init((vector_t)VEC_ZERO, (vector_t)MAX1);
  } else {
    sdl_init((vector_t)VEC_ZERO, (vector_t)MAX_MENU);
  }
  create_map(state->scene, state->game_state);
  sdl_sprites_init(state->scene, state->game_state);
}

void key_event_handler(char key, key_event_type_t type, double held_time,
                       state_t *state) {
  if (state->game_state == MAP1 || state->game_state == MAP2 ||
      state->game_state == MAP3) {
    scene_t *scene = state->scene;
    sprite_t *player1_sprite = fetch_sprite(scene, PLAYER1);
    sprite_t *player2_sprite = fetch_sprite(scene, PLAYER2);
    if ((player1_sprite == NULL || player2_sprite == NULL) &&
        (state->game_state == MAP1 || state->game_state == MAP2)) {
      return;
    }
    body_t *player1 = sprite_get_body(player1_sprite);
    body_t *player2 = sprite_get_body(player2_sprite);
    if (type == KEY_PRESSED) {
      state->key_states[(game_key_t)key] = true;
      switch (key) {
      case A_KEY: {
        get_info(player1)->side = LEFT;
        if (body_get_velocity(player1).x > 0) {
          body_set_velocity(player1,
                            (vector_t){0.0, body_get_velocity(player1).y});
        }
        if (body_get_velocity(player1).x > -PLAYER_MAX_SPEED) {
          body_add_force(player1,
                         (vector_t){.x = -PLAYER_ACCELERATION, .y = 0.0});
        }
        break;
      }
      case D_KEY: {
        get_info(player1)->side = RIGHT;
        if (body_get_velocity(player1).x < 0) {
          body_set_velocity(player1,
                            (vector_t){0.0, body_get_velocity(player1).y});
        }
        if (body_get_velocity(player1).x < PLAYER_MAX_SPEED) {
          body_add_force(player1,
                         (vector_t){.x = PLAYER_ACCELERATION, .y = 0.0});
        }
        break;
      }
      case W_KEY: {
        if (state->time_since_p1_jump > TIME_THRESHOLD) {
          jumping_handler(state, player1);
          state->time_since_p1_jump = 0;
        }
        break;
      }
      case SPACE: {
        player_shoot(state, player1);
        break;
      }
      case LEFT_ARROW: {
        get_info(player2)->side = LEFT;
        if (body_get_velocity(player2).x > 0) {
          body_set_velocity(player2,
                            (vector_t){0.0, body_get_velocity(player2).y});
        }
        if (body_get_velocity(player2).x > -PLAYER_MAX_SPEED) {
          body_add_force(player2,
                         (vector_t){.x = -PLAYER_ACCELERATION, .y = 0.0});
        }
        break;
      }
      case RIGHT_ARROW: {
        get_info(player2)->side = RIGHT;
        if (body_get_velocity(player2).x < 0) {
          body_set_velocity(player2,
                            (vector_t){0.0, body_get_velocity(player2).y});
        }
        if (body_get_velocity(player2).x < PLAYER_MAX_SPEED) {
          body_add_force(player2,
                         (vector_t){.x = PLAYER_ACCELERATION, .y = 0.0});
        }
        break;
      }
      case UP_ARROW: {
        if (state->time_since_p2_jump > TIME_THRESHOLD) {
          jumping_handler(state, player2);
          state->time_since_p2_jump = 0;
        }
        break;
      }
      case PERIOD: {
        player_shoot(state, player2);
        break;
      }
      default:
        break;
      }
    }
    if (type == KEY_RELEASED) {
      state->key_states[(game_key_t)key] = false;
    }
  }

  else if (state->game_state == INTRO_MENU || state->game_state == MAIN_MENU ||
           state->game_state == LORE || state->game_state == MAP_SELECT ||
           state->game_state == CREDITS || state->game_state == INSTRUCTIONS ||
           state->game_state == GAME_WIN_P1 ||
           state->game_state == GAME_WIN_P2) {
    if (type == KEY_PRESSED) {
      state->key_states[(game_key_t)key] = true;
      switch (key) {
      case ONE: {
        if (state->game_state == INTRO_MENU) {
          menu_handler(state, MAIN_MENU);
          break;
        }
        if (state->game_state == MAIN_MENU) {
          menu_handler(state, LORE);
          break;
        }
        if (state->game_state == LORE) {
          menu_handler(state, MAP_SELECT);
          break;
        }
        if (state->game_state == MAP_SELECT) {
          sdl_change_music(state, MAP1_MUS);
          menu_handler(state, MAP1);
          add_lives(state);
          break;
        }
        if (state->game_state == INSTRUCTIONS) {
          menu_handler(state, MAIN_MENU);
          break;
        }
        if (state->game_state == CREDITS) {
          menu_handler(state, MAIN_MENU);
          break;
        }
        if (state->game_state == GAME_WIN_P1) {
          state->p1lives = STARTING_LIVES;
          state->p2lives = STARTING_LIVES;
          state->story_mode = false;
          sdl_change_music(state, MENU_MUS);
          menu_handler(state, MAIN_MENU);
          break;
        }
        if (state->game_state == GAME_WIN_P2) {
          state->p1lives = STARTING_LIVES;
          state->p2lives = STARTING_LIVES;
          state->story_mode = false;
          sdl_change_music(state, MENU_MUS);
          menu_handler(state, MAIN_MENU);
          break;
        }
      }
      case TWO: {
        if (state->game_state == MAIN_MENU) {
          menu_handler(state, INSTRUCTIONS);
          break;
        }
        if (state->game_state == MAP_SELECT) {
          sdl_change_music(state, MAP2_MUS);
          menu_handler(state, MAP2);
          add_lives(state);
          break;
        }
      }
      case THREE: {
        if (state->game_state == MAIN_MENU) {
          menu_handler(state, CREDITS);
          break;
        }
        if (state->game_state == MAP_SELECT) {
          state->story_mode = true;
          sdl_change_music(state, MAP3_MUS);
          menu_handler(state, MAP2);
          add_lives(state);
        }
      }
      default:
        break;
      }
    }
    if (type == KEY_RELEASED) {
      state->key_states[(game_key_t)key] = false;
    }
  }
}

// ---------------------- END KEY EVENTS
// ---------------------------------------------------------------------

void wrap(scene_t *scene) {
  // only applies to map 2
  body_t *player1 = fetch_object(scene, PLAYER1);
  body_t *player2 = fetch_object(scene, PLAYER2);

  if (player1 != NULL && player2 != NULL) {
    check_bounds(player1);
    check_bounds(player2);
  }
}

void reset_map(state_t *state) {
  scene_free(state->scene);
  state->scene = scene_init();

  if (state->story_mode) {
    if (rand() % 2 == 0) {
      state->game_state = MAP3;
    } else {
      state->game_state = MAP2;
    }
  }
  create_map(state->scene, state->game_state);
  sdl_sprites_init(state->scene, state->game_state);
}

bool respawn(state_t *state) {
  if (in_game(state)) {
    if (fetch_object(state->scene, PLAYER1) == NULL && state->p1lives >= 1) {
      state->p1lives -= 1;
      state->time_since_respawn = 0;
      sdl_sound_effects(state, HIT);
      reset_map(state);
      add_lives(state);

      return true;
    }
    if (fetch_object(state->scene, PLAYER2) == NULL && state->p2lives >= 1) {
      state->p2lives -= 1;
      state->time_since_respawn = 0;
      sdl_sound_effects(state, HIT);
      reset_map(state);
      add_lives(state);

      return true;
    }
  }

  return false;
}

// ---------------------- INIT/RUNTIME
// ---------------------------------------------------------------------

state_t *state_init() {
  state_t *state = malloc(sizeof(state_t));
  state->scene = scene_init();
  state->key_states = calloc(NUM_OF_KEYS + 1, sizeof(bool));
  state->sound_effects = sdl_load_sounds();
  state->game_state = INTRO_MENU;
  state->time_since_drop = 0;
  state->time_since_respawn = 0;
  state->time_since_p1_jump = 0;
  state->time_since_p2_jump = 0;
  state->p1lives = STARTING_LIVES;
  state->p2lives = STARTING_LIVES;
  state->story_mode = false;
  return state;
}

list_t *state_get_sounds(state_t *state, size_t idx) {
  return list_get(state->sound_effects, idx);
}

state_t *emscripten_init(void) {
  srand(time(NULL));

  state_t *state = state_init();
  create_map(state->scene, state->game_state);

  if (state->game_state == MAP2) {
    sdl_init((vector_t)VEC_ZERO, (vector_t)MAX2);
  } else if (state->game_state == MAP1) {
    sdl_init((vector_t)VEC_ZERO, (vector_t)MAX1);
  } else {
    sdl_init((vector_t)VEC_ZERO, (vector_t)MAX_MENU);
  }

  sdl_sprites_init(state->scene, state->game_state);
  sdl_on_key(key_event_handler);
  sdl_music(state, MENU_MUS);
  return state;
}

void apply_time(state_t *state, double dt, body_t *player1, body_t *player2) {
  state->time_since_drop += TIME_MULT * dt;
  state->time_since_respawn += TIME_MULT * dt;
  state->time_since_p1_jump += TIME_MULT * dt;
  state->time_since_p2_jump += TIME_MULT * dt;
  if (player1 != NULL && player2 != NULL) {
    get_info(player1)->time_since_last_shot += TIME_MULT * dt;
    get_info(player2)->time_since_last_shot += TIME_MULT * dt;
  }
}

void emscripten_main(state_t *state) {
  double dt = time_since_last_tick();
  body_t *player1 = fetch_object(state->scene, PLAYER1);
  body_t *player2 = fetch_object(state->scene, PLAYER2);

  apply_time(state, dt, player1, player2);
  if (player1 != NULL && player2 != NULL) {
    apply_key_states(state, player1, player2);
  }

  // Clock
  if (state->game_state == MAP2 || state->game_state == MAP3) {
    body_t *clock_big_arm = fetch_object(state->scene, CLOCK_BIG_ARM);
    body_t *clock_small_arm = fetch_object(state->scene, CLOCK_SMALL_ARM);
    if (clock_big_arm != NULL && clock_small_arm != NULL) {
      double angle1 = body_get_angle(clock_big_arm);
      double angle2 = body_get_angle(clock_small_arm);
      if (fabs(angle1 - angle2) < ANGLE_ERROR) {
        body_set_rot_acceleration(clock_big_arm,
                                  body_get_rot_acceleration(clock_big_arm) *
                                      ANGULAR_MULTIPLIER_BIG);
        body_set_rot_acceleration(clock_small_arm,
                                  body_get_rot_acceleration(clock_small_arm) *
                                      ANGULAR_MULTIPLIER_SMALL);
      }
    }
  }

  // Powerups
  if (state->game_state == MAP1 || state->game_state == MAP2 ||
      state->game_state == MAP3) {
    size_t powerups_on_screen = 0;
    for (size_t i = 0; i < scene_bodies(state->scene); i++) {
      body_type_t type = get_info(scene_get_body(state->scene, i))->type;
      if (type == POWERUP_RICOCHET || type == POWERUP_SHOTGUN) {
        powerups_on_screen++;
        if (powerups_on_screen >= MAX_POWERUPS) {
          break;
        }
      }
    }
    bool spawned = spawn_powerup(state->scene, state->time_since_drop,
                                 powerups_on_screen, state->game_state);
    if (spawned) {
      state->time_since_drop = 0;
    }
  }

  // Wrap
  if (state->game_state == MAP2 || state->game_state == MAP3) {
    wrap(state->scene);
  }

  // Tick and Reset
  scene_tick(state->scene, dt);
  if (((!respawn(state)) && state->time_since_respawn > TIME_THRESHOLD) ||
      !in_game(state)) {
    sdl_render_game(state->scene);
  }

  if (state->game_state == MAP1 || state->game_state == MAP2 ||
      state->game_state == MAP3) {
    if (state->p1lives == 0) {
      sdl_change_music(state, END_MUS);
      menu_handler(state, GAME_WIN_P2);
    }
    if (state->p2lives == 0) {
      sdl_change_music(state, END_MUS);
      menu_handler(state, GAME_WIN_P1);
    }
  }
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}

// ---------------------- END INIT/RUNTIME
// ---------------------------------------------------------------------