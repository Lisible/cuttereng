#include "cuttereng.h"
#include "environment.h"
#include "filesystem.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>

void cutter_bootstrap() {
  LOG_INFO("Bootstrapping...");
  char *configuration_file_path = env_get_configuration_file_path();
  LOG_DEBUG("Configuration file path: %s", configuration_file_path);
  char *configuration_file_content =
      read_file_to_string(configuration_file_path);
  LOG_DEBUG("Configuration file content:\n%s", configuration_file_content);

  free(configuration_file_path);
  free(configuration_file_content);
}
