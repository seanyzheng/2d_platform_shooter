#include "sdl_wrapper.h"
#include "list.h"
#include "map.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char WINDOW_TITLE[] = "CS 3";
const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 500;
const double MS_PER_S = 1e3;
const int FREQUENCY = 44100;
const int CHANNELS = 2;
const int CHUNKSIZE = 1024;

/**
 * The coordinate at the center of the screen.
 */
vector_t center;
/**
 * The coordinate difference from the center to the top right corner.
 */
vector_t max_diff;
/**
 * The SDL window where the scene is rendered.
 */
SDL_Window *window;
/**
 * The renderer used to draw the scene.
 */
SDL_Renderer *renderer;
/**
 * The keypress handler, or NULL if none has been configured.
 */
key_handler_t key_handler = NULL;
/**
 * SDL's timestamp when a key was last pressed or released.
 * Used to mesasure how long a key has been held.
 */
uint32_t key_start_timestamp;
/**
 * The value of clock() when time_since_last_tick() was last called.
 * Initially 0.
 */
clock_t last_clock = 0;

/** Computes the center of the window in pixel coordinates */
vector_t get_window_center(void) {
  int *width = malloc(sizeof(*width)), *height = malloc(sizeof(*height));
  assert(width != NULL);
  assert(height != NULL);
  SDL_GetWindowSize(window, width, height);
  vector_t dimensions = {.x = *width, .y = *height};
  free(width);
  free(height);
  return vec_multiply(0.5, dimensions);
}

/**
 * Computes the scaling factor between scene coordinates and pixel coordinates.
 * The scene is scaled by the same factor in the x and y dimensions,
 * chosen to maximize the size of the scene while keeping it in the window.
 */
double get_scene_scale(vector_t window_center) {
  // Scale scene so it fits entirely in the window
  double x_scale = window_center.x / max_diff.x,
         y_scale = window_center.y / max_diff.y;
  return x_scale < y_scale ? x_scale : y_scale;
}

/** Maps a scene coordinate to a window coordinate */
vector_t get_window_position(vector_t scene_pos, vector_t window_center) {
  // Scale scene coordinates by the scaling factor
  // and map the center of the scene to the center of the window
  vector_t scene_center_offset = vec_subtract(scene_pos, center);
  double scale = get_scene_scale(window_center);
  vector_t pixel_center_offset = vec_multiply(scale, scene_center_offset);
  vector_t pixel = {.x = round(window_center.x + pixel_center_offset.x),
                    // Flip y axis since positive y is down on the screen
                    .y = round(window_center.y - pixel_center_offset.y)};
  return pixel;
}

/**
 * Converts an SDL key code to a char.
 * 7-bit ASCII characters are just returned
 * and arrow keys are given special character codes.
 */
char get_keycode(SDL_Keycode key) {
  switch (key) {
  case SDLK_LEFT:
    return LEFT_ARROW;
  case SDLK_UP:
    return UP_ARROW;
  case SDLK_RIGHT:
    return RIGHT_ARROW;
  case SDLK_DOWN:
    return DOWN_ARROW;
  case SDLK_SPACE:
    return SPACE;
  case SDLK_w:
    return W_KEY;
  case SDLK_a:
    return A_KEY;
  case SDLK_s:
    return S_KEY;
  case SDLK_d:
    return D_KEY;
  case SDLK_PERIOD:
    return PERIOD;
  case SDLK_1:
    return ONE;
  case SDLK_2:
    return TWO;
  case SDLK_3:
    return THREE;

  default:
    // Only process 7-bit ASCII characters
    return key == (SDL_Keycode)(char)key ? key : '\0';
  }
}

bool sdl_is_done(void *state) {
  SDL_Event *event = malloc(sizeof(*event));
  assert(event != NULL);
  while (SDL_PollEvent(event)) {
    switch (event->type) {
    case SDL_QUIT:
      free(event);
      return true;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      // Skip the keypress if no handler is configured
      // or an unrecognized key was pressed
      if (key_handler == NULL)
        break;
      char key = get_keycode(event->key.keysym.sym);
      if (key == '\0')
        break;

      uint32_t timestamp = event->key.timestamp;
      if (!event->key.repeat) {
        key_start_timestamp = timestamp;
      }
      key_event_type_t type =
          event->type == SDL_KEYDOWN ? KEY_PRESSED : KEY_RELEASED;
      double held_time = (timestamp - key_start_timestamp) / MS_PER_S;
      key_handler(key, type, held_time, (state_t *)state);
      break;
    }
  }
  free(event);
  return false;
}

void sdl_clear(void) {
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);
}

void sdl_draw_polygon(list_t *points, rgb_color_t color) {
  // Check parameters
  size_t n = list_size(points);
  assert(n >= 3);
  assert(0 <= color.r && color.r <= 1);
  assert(0 <= color.g && color.g <= 1);
  assert(0 <= color.b && color.b <= 1);

  vector_t window_center = get_window_center();

  // Convert each vertex to a point on screen
  int16_t *x_points = malloc(sizeof(*x_points) * n),
          *y_points = malloc(sizeof(*y_points) * n);
  assert(x_points != NULL);
  assert(y_points != NULL);
  for (size_t i = 0; i < n; i++) {
    vector_t *vertex = list_get(points, i);
    vector_t pixel = get_window_position(*vertex, window_center);
    x_points[i] = pixel.x;
    y_points[i] = pixel.y;
  }

  // Draw polygon with the given color
  filledPolygonRGBA(renderer, x_points, y_points, n, color.r * 255,
                    color.g * 255, color.b * 255, 255);
  free(x_points);
  free(y_points);
}

void sdl_change_music(state_t *state, sound_t sound) {
  Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
  Mix_HaltMusic();
  sdl_music(state, sound);
}

list_t *sdl_load_sounds(void) {
  list_t *sound_effects = list_init(3, NULL);
  Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
  Mix_Chunk *pistol = Mix_LoadWAV("assets/pistol.wav");
  Mix_Chunk *shotgun = Mix_LoadWAV("assets/shotgun.wav");
  Mix_Chunk *ricochet = Mix_LoadWAV("assets/ricochet.wav");
  Mix_Chunk *jump = Mix_LoadWAV("assets/jump.wav");
  Mix_Chunk *click = Mix_LoadWAV("assets/menu1.wav");
  Mix_Chunk *hit = Mix_LoadWAV("assets/hit.wav");
  Mix_Music *menu_mus = Mix_LoadMUS("assets/menu_mus.wav");
  Mix_Music *map1_mus = Mix_LoadMUS("assets/map1_music.wav");
  Mix_Music *map2_mus = Mix_LoadMUS("assets/map2_music.wav");
  Mix_Music *end_mus = Mix_LoadMUS("assets/end_music.wav");
  Mix_Music *map3_mus = Mix_LoadMUS("assets/map3_music.wav");

  list_add(sound_effects, pistol);
  list_add(sound_effects, shotgun);
  list_add(sound_effects, ricochet);
  list_add(sound_effects, jump);
  list_add(sound_effects, click);
  list_add(sound_effects, hit);
  list_add(sound_effects, menu_mus);
  list_add(sound_effects, map1_mus);
  list_add(sound_effects, map2_mus);
  list_add(sound_effects, end_mus);
  list_add(sound_effects, map3_mus);
  return sound_effects;
}

void sdl_music(state_t *state, sound_t sound) {
  Mix_OpenAudio(FREQUENCY, MIX_DEFAULT_FORMAT, CHANNELS, CHUNKSIZE);
  Mix_PlayMusic((Mix_Music *)state_get_sounds(state, sound), -1);
}

void sdl_sound_effects(state_t *state, sound_t sound) {
  Mix_OpenAudio(FREQUENCY, MIX_DEFAULT_FORMAT, CHANNELS, CHUNKSIZE);
  Mix_PlayChannel(-1, (Mix_Chunk *)state_get_sounds(state, sound), 0);
}

void sdl_sprites_init(scene_t *scene, game_state_t state) {
  sprite_list_init(scene);
  sprite_img_init(scene, state);
}

void sprite_img_init(scene_t *scene, game_state_t state) {
  size_t MAX_PATH_LENGTH = 50;
  size_t name_len = 8;
  size_t assets_len = 7;
  size_t prefix_length = name_len + assets_len;

  char state_name[name_len];
  if (state == MAP1 || state == MAP3) {
    strcpy(state_name, "map1_");
  } else if (state == MAP2) {
    strcpy(state_name, "map2_");
  } else if (state == INTRO_MENU) {
    strcpy(state_name, "intr_");
  } else if (state == MAIN_MENU) {
    strcpy(state_name, "main_");
  } else if (state == LORE) {
    strcpy(state_name, "lore_");
  } else if (state == MAP_SELECT) {
    strcpy(state_name, "slec_");
  } else if (state == CREDITS) {
    strcpy(state_name, "cred_");
  } else if (state == INSTRUCTIONS) {
    strcpy(state_name, "inst_");
  } else if (state == GAME_WIN_P1) {
    strcpy(state_name, "end1_");
  } else if (state == GAME_WIN_P2) {
    strcpy(state_name, "end2_");
  }

  size_t sprite_count = list_size(scene_get_sprites(scene));
  for (size_t i = 0; i < sprite_count; i++) {
    sprite_t *sprite = scene_get_sprite(scene, i);
    body_t *body = sprite_get_body(sprite);
    list_t *surfaces = list_init(1, (free_func_t)SDL_FreeSurface);

    char path[MAX_PATH_LENGTH];
    strcpy(path, "assets/");
    strcat(path, state_name);

    switch (((body_info_t *)body_get_info(body))->type) {
    case PLAYER1: {
      char path_suffix[9] = "p1_0.png";
      strcat(path, path_suffix);
      list_add(surfaces, IMG_Load(path));

      path[prefix_length] = '1';
      list_add(surfaces, IMG_Load(path));

      path[prefix_length] = '2';
      list_add(surfaces, IMG_Load(path));

      path[prefix_length] = '3';
      list_add(surfaces, IMG_Load(path));

      break;
    }
    case PLAYER2: {
      char path_suffix[9] = "p2_0.png";
      strcat(path, path_suffix);
      list_add(surfaces, IMG_Load(path));

      path[prefix_length] = '1';
      list_add(surfaces, IMG_Load(path));

      path[prefix_length] = '2';
      list_add(surfaces, IMG_Load(path));

      path[prefix_length] = '3';
      list_add(surfaces, IMG_Load(path));

      break;
    }
    case GROUND: {
      char path_suffix[11] = "ground.png";
      strcat(path, path_suffix);
      list_add(surfaces, IMG_Load(path));
      break;
    }
    case WALL: {
      char path_suffix[9] = "wall.png";
      strcat(path, path_suffix);
      list_add(surfaces, IMG_Load(path));
      break;
    }
    case BACKGROUND: {
      char path_suffix[15] = "background.jpg";
      strcat(path, path_suffix);
      list_add(surfaces, IMG_Load(path));
      break;
    }
    default:
      break;
    }

    // for each surface in list
    for (size_t i = 0; i < list_size(surfaces); i++) {
      SDL_Surface *surface = (SDL_Surface *)list_get(surfaces, i);
      SDL_SetColorKey(surface, SDL_TRUE,
                      SDL_MapRGB(surface->format, 255, 255, 255));
      SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);
      if (tex != NULL) {
        sprite_add_tex(sprite, tex);
      }
    }

    list_free(surfaces);
  }
}

void sprite_img_add(scene_t *scene, body_t *body, game_state_t state) {
  sprite_t *sprite = sprite_init(body);
  body_info_t *info = get_info(body);
  SDL_Surface *surface = NULL;
  switch (info->type) {
  case POWERUP_RICOCHET:
    surface = IMG_Load("assets/powerup_ricochet.png");
    break;
  case POWERUP_SHOTGUN:
    surface = IMG_Load("assets/powerup_shotgun.png");
    break;
  case P1_LIFE:
    if (state == MAP1 || state == MAP3) {
      surface = IMG_Load("assets/p1_life.png");
    } else if (state == MAP2) {
      surface = IMG_Load("assets/p1_life.png");
    }
    break;
  case P2_LIFE:
    if (state == MAP1 || state == MAP3) {
      surface = IMG_Load("assets/p2_life.png");
    } else if (state == MAP2) {
      surface = IMG_Load("assets/p2_life.png");
    }
    break;
  default:
    break;
  }
  assert(surface != NULL);
  SDL_SetColorKey(surface, SDL_TRUE,
                  SDL_MapRGB(surface->format, 255, 255, 255));
  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  sprite_add_tex(sprite, tex);
  scene_add_sprite(scene, sprite);
}

void sprite_img_update(sprite_t *sprite) {
  const size_t WALKING_MOD = 38;
  const size_t RUNNING_MOD = 12;
  body_type_t body_type = get_info(sprite_get_body(sprite))->type;
  assert(body_type == PLAYER1 || body_type == PLAYER2);

  size_t new_frame = sprite_get_curr_ind(sprite);
  vector_t vel = body_get_velocity(sprite_get_body(sprite));

  if (vel.y > 0) {
    new_frame = 1;
  } else if (vel.y < 0) {
    new_frame = 3;
  } else if (vel.y == 0) {
    size_t mod = WALKING_MOD;
    if (vel.x != 0) {
      mod = RUNNING_MOD;
    }
    if (rand() % mod == 0) {
      new_frame = (new_frame + 1) % sprite_textures(sprite);
    }
  }

  sprite_set_tex(sprite, new_frame);
}

void sdl_show(void) {
  // Draw boundary lines
  vector_t window_center = get_window_center();
  vector_t max = vec_add(center, max_diff),
           min = vec_subtract(center, max_diff);
  vector_t max_pixel = get_window_position(max, window_center),
           min_pixel = get_window_position(min, window_center);
  SDL_Rect *boundary = malloc(sizeof(*boundary));
  boundary->x = min_pixel.x;
  boundary->y = max_pixel.y;
  boundary->w = max_pixel.x - min_pixel.x;
  boundary->h = min_pixel.y - max_pixel.y;
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderDrawRect(renderer, boundary);
  free(boundary);

  SDL_RenderPresent(renderer);
}

void sdl_init(vector_t min, vector_t max) {
  // Check parameters
  assert(min.x < max.x);
  assert(min.y < max.y);

  center = vec_multiply(0.5, vec_add(min, max));
  max_diff = vec_subtract(max, center);
  SDL_Init(SDL_INIT_EVERYTHING);
  window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                            SDL_WINDOW_RESIZABLE);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
}

void sdl_clean(void) {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}

void sdl_render_game(scene_t *scene) {
  sdl_clear();
  sprite_list_update(scene);
  size_t sprite_count = list_size(scene_get_sprites(scene));
  sprite_t *player1_sprite = NULL;
  sprite_t *player2_sprite = NULL;
  for (size_t i = 0; i < sprite_count; i++) {
    sprite_t *sprite = scene_get_sprite(scene, i);
    body_info_t *info = get_info(sprite_get_body(sprite));

    if (info->type == PLAYER1) {
      player1_sprite = sprite;
    } else if (info->type == PLAYER2) {
      player2_sprite = sprite;
    }

    SDL_RendererFlip flip = SDL_FLIP_NONE;
    if (sprite_textures(sprite) != 0) {
      if (info->side == LEFT) {
        flip = SDL_FLIP_HORIZONTAL;
      }

      SDL_RenderCopyEx(renderer,
                       sprite_get_tex(sprite, sprite_get_curr_ind(sprite)),
                       NULL, sprite_get_destR(sprite), 0, NULL, flip);
    }
  }

  size_t body_count = scene_bodies(scene);
  for (size_t i = 0; i < body_count; i++) {
    body_t *body = scene_get_body(scene, i);
    list_t *shape = body_get_shape(body);
    body_type_t type = get_info(body)->type;
    if (type == BULLET || type == CLOCK || type == CLOCK_BIG_ARM ||
        type == CLOCK_SMALL_ARM) {
      sdl_draw_polygon(shape, body_get_color(body));
    }
    list_free(shape);
  }

  if (player1_sprite != NULL) {
    body_info_t *info = get_info(sprite_get_body(player1_sprite));
    sprite_img_update(player1_sprite);

    SDL_RendererFlip flip = SDL_FLIP_NONE;
    if (sprite_textures(player1_sprite) != 0) {
      if (info->side == LEFT) {
        flip = SDL_FLIP_HORIZONTAL;
      }

      SDL_RenderCopyEx(
          renderer,
          sprite_get_tex(player1_sprite, sprite_get_curr_ind(player1_sprite)),
          NULL, sprite_get_destR(player1_sprite), 0, NULL, flip);
    }
  }

  if (player2_sprite != NULL) {
    body_info_t *info = get_info(sprite_get_body(player2_sprite));
    sprite_img_update(player2_sprite);

    SDL_RendererFlip flip = SDL_FLIP_NONE;
    if (sprite_textures(player2_sprite) != 0) {
      if (info->side == LEFT) {
        flip = SDL_FLIP_HORIZONTAL;
      }

      SDL_RenderCopyEx(
          renderer,
          sprite_get_tex(player2_sprite, sprite_get_curr_ind(player2_sprite)),
          NULL, sprite_get_destR(player2_sprite), 0, NULL, flip);
    }
  }

  sdl_show();
}

void sdl_render_scene(scene_t *scene) {
  sdl_clear();
  size_t body_count = scene_bodies(scene);
  for (size_t i = 0; i < body_count; i++) {
    body_t *body = scene_get_body(scene, i);
    list_t *shape = body_get_shape(body);
    sdl_draw_polygon(shape, body_get_color(body));
    list_free(shape);
  }
  sdl_show();
}

void sdl_on_key(key_handler_t handler) { key_handler = handler; }

double time_since_last_tick(void) {
  clock_t now = clock();
  double difference = last_clock
                          ? (double)(now - last_clock) / CLOCKS_PER_SEC
                          : 0.0; // return 0 the first time this is called
  last_clock = now;
  return difference;
}
