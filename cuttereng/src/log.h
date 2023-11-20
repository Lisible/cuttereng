#ifndef CUTTERENG_LOG_H
#define CUTTERENG_LOG_H

#include <stdarg.h>
#include <stdio.h>

typedef enum log_level_t {
	TRACE,
	DEBUG,
	WARN,
	ERROR,
	INFO
} log_level_t;

#define LOG_TRACE(format_str, ...) LOG(TRACE, format_str, ##__VA_ARGS__)
#define LOG_DEBUG(format_str, ...) LOG(DEBUG, format_str, ##__VA_ARGS__)
#define LOG_WARN(format_str, ...) LOG(WARN, format_str, ##__VA_ARGS__)
#define LOG_ERROR(format_str, ...) LOG(ERROR, format_str, ##__VA_ARGS__)
#define LOG_INFO(format_str, ...) LOG(INFO, format_str, ##__VA_ARGS__)
#define LOG(log_level, format_str, ...) \
	do { log_message(log_level, __FILE__, __LINE__, "[%s %s:%d] " format_str "\n", ##__VA_ARGS__); } while(0)

void log_message(log_level_t log_level, const char* filename, int line_number, const char* format_str, ...);
const char* log_level_to_string(log_level_t log_level);

#endif // CUTTERENG_LOG_H
