#include <stdio.h>
#include "cuttereng.h"
#include "platform.h"

void cutter_bootstrap() {
	char executable_path[4096] = {0};
	platform_get_process_executable(executable_path, 4096);
	printf("%s\n", executable_path);
}

