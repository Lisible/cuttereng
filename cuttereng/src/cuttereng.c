#include "cuttereng.h"
#include "fs.h"
#include "log.h"
#include "platform.h"
#include <stdio.h>

void cutter_bootstrap() {
  LOG_INFO("Bootstrapping...");
  char executable_path[4096] = {0};
  platform_get_process_executable(executable_path, 4096);
  LOG_DEBUG("executable directory: %s", executable_path);
}
