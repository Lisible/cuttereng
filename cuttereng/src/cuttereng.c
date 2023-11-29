#include "cuttereng.h"
#include "environment.h"
#include "filesystem.h"
#include "log.h"
#include "src/json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  unsigned int width;
  unsigned int height;
} window_size;

bool window_size_from_json(json *json, window_size *window_size);
void window_size_debug_print(window_size *window_size);

typedef struct {
  char *application_title;
  window_size window_size;
} configuration;

/// Creates a configuration struct from a configuration json value
///
/// Note: This takes ownership of `configuration_json`
/// @return true in case of success, false otherwise
bool configuration_from_json(json *configuration_json,
                             configuration *output_configuration);
void configuration_debug_print(configuration *configuration);
void configuration_cleanup(configuration *configuration);

void cutter_bootstrap() {
  LOG_INFO("Bootstrapping...");
  char *configuration_file_path = env_get_configuration_file_path();
  LOG_DEBUG("Configuration file path: %s", configuration_file_path);
  char *configuration_file_content =
      read_file_to_string(configuration_file_path);

  json *configuration_json = json_parse_from_str(configuration_file_content);
  if (!configuration_json)
    goto cleanup;

  configuration config;
  if (!configuration_from_json(configuration_json, &config))
    goto cleanup_2;

  configuration_debug_print(&config);
  configuration_cleanup(&config);
cleanup_2:
  json_destroy(configuration_json);
cleanup:
  free(configuration_file_path);
  free(configuration_file_content);
}

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

void configuration_cleanup(configuration *configuration) {
  free(configuration->application_title);
  configuration->application_title = NULL;
}
