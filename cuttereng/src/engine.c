#include "engine.h"
#include "src/log.h"
#include "src/memory.h"

void engine_init(engine *engine, const configuration *configuration) {
  engine->application_title = configuration->application_title;
  engine->running = true;
}

void engine_deinit(engine *engine) {
  memory_free((void *)engine->application_title);
}

void engine_handle_events(engine *engine, event *event) {
  switch (event->type) {
  case EVT_QUIT:
    engine->running = false;
    break;
  default:
    break;
  }
}
void engine_update(engine *engine) { LOG_INFO("update"); }
void engine_render(engine *engine) { LOG_INFO("render"); }
bool engine_is_running(engine *engine) { return engine->running; }

bool configuration_from_json(json *configuration_json,
                             configuration *output_configuration) {

  if (configuration_json->type != JSON_OBJECT) {
    LOG_ERROR("project_configuration.json's root should be an object");
    return false;
  }
  json_object *configuration_json_object = configuration_json->object;
  json *title_json = json_object_get(configuration_json_object, "title");
  if (!title_json) {
    LOG_ERROR("no property title found in project_configuration.json");
    return false;
  }

  if (title_json->type != JSON_STRING) {
    LOG_ERROR("title is not a string");
    return false;
  }

  output_configuration->application_title = title_json->string;
  json_object_steal(configuration_json_object, "title");
  json_destroy_without_cleanup(title_json);

  json *window_size_json =
      json_object_get(configuration_json->object, "window_size");
  if (!window_size_json) {
    LOG_ERROR("no window_size property found in project_configuration.json");
    return false;
  }

  if (!window_size_from_json(window_size_json,
                             &output_configuration->window_size)) {
    return false;
  }

  return true;
}

bool window_size_from_json(json *json_value, window_size *window_size) {

  if (json_value->type != JSON_OBJECT) {
    LOG_ERROR("window_size property is not an object");
    return false;
  }

  json *width = json_object_get(json_value->object, "width");
  if (!width) {
    LOG_ERROR("window_size.width not found");
    return false;
  }

  if (width->type != JSON_NUMBER) {
    LOG_ERROR("window_size.width should be a number");
    return false;
  }
  window_size->width = width->number;

  json *height = json_object_get(json_value->object, "height");
  if (!height) {
    LOG_ERROR("window_size.height not found");
    return false;
  }
  if (height->type != JSON_NUMBER) {
    LOG_ERROR("window_size.height should be a number");
    return false;
  }
  window_size->height = height->number;
  return true;
}

void configuration_debug_print(configuration *configuration) {
  LOG_DEBUG("Configuration");
  LOG_DEBUG("title: %s", configuration->application_title);
  window_size_debug_print(&configuration->window_size);
}
void window_size_debug_print(window_size *window_size) {
  LOG_DEBUG("window size: (%d, %d)", window_size->width, window_size->height);
}
