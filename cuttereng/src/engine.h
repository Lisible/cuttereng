#ifndef CUTTERENG_ENGINE_H
#define CUTTERENG_ENGINE_H

#include "asset.h"
#include "ecs/ecs.h"
#include "event.h"
#include "input.h"
#include "json.h"
#include "math/matrix.h"
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
  Ecs ecs;
  InputState input_state;
  Assets *assets;
  const char *application_title;
  float current_time_secs;
  mat4 *transform_cache;
  bool running;
  bool capturing_mouse;
} Engine;

typedef struct {
  float delta_time_secs;
  float current_time_secs;
  InputState *input_state;
  Assets *assets;
} SystemContext;

void engine_init(Engine *engine, const Configuration *config,
                 EcsSystemFn init_system, SDL_Window *window);
void engine_set_current_time(Engine *engine, float current_time_secs);
void engine_deinit(Engine *engine);
void engine_handle_events(Engine *engine, Event *event);
void engine_update(Allocator *frame_allocator, Engine *engine, float dt);
void engine_render(Allocator *frame_allocator, Engine *engine);
bool engine_is_running(Engine *engine);
bool engine_should_mouse_be_captured(Engine *engine);

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
