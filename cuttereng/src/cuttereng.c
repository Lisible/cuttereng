#include "cuttereng.h"
#include "SDL_events.h"
#include "assert.h"
#include "engine.h"
#include "environment/environment.h"
#include "event.h"
#include "filesystem.h"
#include "json.h"
#include "log.h"
#include "src/memory.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const size_t KB = 1000;

void event_from_sdl_event(SDL_Event *sdl_event, Event *event);

void cutter_bootstrap(EcsInitSystem ecs_init_system) {
  LOG_INFO("Bootstrapping...");
  char *configuration_file_path =
      env_get_configuration_file_path(&system_allocator);
  LOG_DEBUG("Configuration file path: %s", configuration_file_path);
  char *configuration_file_content = filesystem_read_file_to_string(
      &system_allocator, configuration_file_path);

  Json *configuration_json =
      json_parse_from_str(&system_allocator, configuration_file_content);
  if (!configuration_json)
    goto cleanup;

  Configuration config;
  if (!configuration_from_json(configuration_json, &config))
    goto cleanup_2;

  Engine engine;
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window = SDL_CreateWindow(
      config.application_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      config.window_size.width, config.window_size.height, 0);
  engine_init(&engine, &config, ecs_init_system, window);

  Arena frame_arena;
  Allocator frame_allocator = arena_allocator(&frame_arena);
  arena_init(&frame_arena, &system_allocator, 10 * KB);
  while (engine_is_running(&engine)) {
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event) != 0) {
      Event event;
      event_from_sdl_event(&sdl_event, &event);
      engine_handle_events(&engine, &event);
    }
    engine_set_current_time(&engine, SDL_GetTicks() / 1000.f);
    engine_update(&frame_allocator, &engine);
    engine_render(&frame_allocator, &engine);
    arena_clear(&frame_arena);
  }
  arena_deinit(&frame_arena, &system_allocator);

  SDL_DestroyWindow(window);
  engine_deinit(&engine);
  SDL_Quit();

cleanup_2:
  json_destroy(&system_allocator, configuration_json);
cleanup:
  free(configuration_file_path);
  free(configuration_file_content);
}

Key key_from_sdl_keysym(SDL_Keycode keycode) {
  switch (keycode) {
  case SDLK_a:
    return Key_A;
  case SDLK_b:
    return Key_B;
  case SDLK_c:
    return Key_C;
  case SDLK_d:
    return Key_D;
  case SDLK_e:
    return Key_E;
  case SDLK_f:
    return Key_F;
  case SDLK_g:
    return Key_G;
  case SDLK_h:
    return Key_H;
  case SDLK_i:
    return Key_I;
  case SDLK_j:
    return Key_J;
  case SDLK_k:
    return Key_K;
  case SDLK_l:
    return Key_L;
  case SDLK_m:
    return Key_M;
  case SDLK_n:
    return Key_N;
  case SDLK_o:
    return Key_O;
  case SDLK_p:
    return Key_P;
  case SDLK_q:
    return Key_Q;
  case SDLK_r:
    return Key_R;
  case SDLK_s:
    return Key_S;
  case SDLK_t:
    return Key_T;
  case SDLK_u:
    return Key_U;
  case SDLK_v:
    return Key_V;
  case SDLK_w:
    return Key_W;
  case SDLK_x:
    return Key_X;
  case SDLK_y:
    return Key_Y;
  case SDLK_z:
    return Key_Z;
  default:
    return Key_Unknown;
  }
}

void event_from_sdl_event(SDL_Event *sdl_event, Event *event) {
  ASSERT(sdl_event != NULL);
  ASSERT(event != NULL);

  switch (sdl_event->type) {
  case SDL_QUIT:
    event->type = EVT_QUIT;
    break;
  case SDL_KEYDOWN:
    event->type = EVT_KEYDOWN;
    event->key_event.key = key_from_sdl_keysym(sdl_event->key.keysym.sym);
    break;
  case SDL_KEYUP:
    event->type = EVT_KEYUP;
    event->key_event.key = key_from_sdl_keysym(sdl_event->key.keysym.sym);
    break;
  case SDL_MOUSEMOTION:
    event->type = EVT_MOUSEMOTION;
    event->mouse_motion_event.relative_x = sdl_event->motion.xrel;
    event->mouse_motion_event.relative_y = sdl_event->motion.yrel;
    break;
  default:
    event->type = EVT_UNKNOWN;
    break;
  }
}
