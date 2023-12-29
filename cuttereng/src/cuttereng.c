#include "cuttereng.h"
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

void event_from_sdl_event(SDL_Event *sdl_event, Event *event);

void cutter_bootstrap() {
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
  engine_init(&engine, &config, window);

  while (engine_is_running(&engine)) {
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event) != 0) {
      Event event;
      event_from_sdl_event(&sdl_event, &event);
      engine_handle_events(&engine, &event);
    }
    engine_set_current_time(&engine, SDL_GetTicks() / 1000.f);
    engine_update(&engine);
    engine_render(&engine);
  }

  SDL_DestroyWindow(window);
  engine_deinit(&engine);
  SDL_Quit();

cleanup_2:
  json_destroy(&system_allocator, configuration_json);
cleanup:
  free(configuration_file_path);
  free(configuration_file_content);
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
    break;
  default:
    event->type = EVT_UNKNOWN;
    break;
  }
}
