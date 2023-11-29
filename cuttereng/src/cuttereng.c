#include "cuttereng.h"
#include "engine.h"
#include "environment.h"
#include "filesystem.h"
#include "json.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

  engine engine;
  engine_init(&engine, &config);
  while (true) {
    engine_handle_events(&engine);
    engine_update(&engine);
    engine_render(&engine);
  }

  engine_deinit(&engine);

cleanup_2:
  json_destroy(configuration_json);
cleanup:
  free(configuration_file_path);
  free(configuration_file_content);
}
