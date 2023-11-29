#ifndef CUTTERENG_ENGINE_H
#define CUTTERENG_ENGINE_H

#include "common.h"
#include "json.h"

typedef struct configuration configuration;
typedef struct engine engine;
struct engine {
  const char *application_title;
};
void engine_init(engine *engine, const configuration *config);
void engine_deinit(engine *engine);
void engine_handle_events(engine *engine);
void engine_update(engine *engine);
void engine_render(engine *engine);

typedef struct window_size window_size;
struct window_size {
  unsigned int width;
  unsigned int height;
};
bool window_size_from_json(json *json, window_size *window_size);
void window_size_debug_print(window_size *window_size);

struct configuration {
  char *application_title;
  window_size window_size;
};

/// Creates a configuration struct from a configuration json value
///
/// Note: This takes ownership of `configuration_json`
/// @return true in case of success, false otherwise
bool configuration_from_json(json *configuration_json,
                             configuration *output_configuration);
void configuration_debug_print(configuration *configuration);

#endif // CUTTERENG_ENGINE_H
