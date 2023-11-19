#include "../platform.h"

void platform_get_process_executable(char* const output_string, size_t output_string_length) {
	ssize_t size_read = readlink("/proc/self/exe", output_string, output_string_length);
	output_string[output_string_length - 1] = '\0';
}
