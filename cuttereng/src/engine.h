#ifndef CUTTERENG_ENGINE_H
#define CUTTERENG_ENGINE_H

#include "asset.h"
#include "common.h"
#include "event.h"
#include "json.h"
#include "renderer/renderer.h"
#include <SDL.h>

typedef struct {
  unsigned int width;
  unsigned int height;
} WindowSize;

typedef struct {
  char *application_title;
  WindowSize window_size;
} Configuration;

typedef struct {
  Renderer *renderer;
  Assets *assets;
  const char *application_title;
  bool running;
} Engine;

void engine_init(Engine *engine, const Configuration *config,
                 SDL_Window *window);
void engine_deinit(Engine *engine);
void engine_handle_events(Engine *engine, Event *event);
void engine_update(Engine *engine);
void engine_render(Engine *engine);
bool engine_is_running(Engine *engine);

bool window_size_from_json(Json *json, WindowSize *window_size);
void window_size_debug_print(WindowSize *window_size);

/// Creates a configuration struct from a configuration json value
///
/// Note: This takes ownership of `configuration_json`
/// @return true in case of success, false otherwise
bool configuration_from_json(Json *configuration_json,
                             Configuration *output_configuration);
void configuration_debug_print(Configuration *configuration);

#endif // CUTTERENG_ENGINE_H
