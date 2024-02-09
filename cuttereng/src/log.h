#ifndef CUTTERENG_LOG_H
#define CUTTERENG_LOG_H

#include <stdarg.h>
#include <stdio.h>

typedef enum { TRACE, DBG, WARN, ERROR, INFO } LogLevel;
#define LOG_LEVEL DBG

#define LOG_TRACE(format_str, ...) LOG(TRACE, format_str, ##__VA_ARGS__)
#define LOG_DEBUG(format_str, ...) LOG(DBG, format_str, ##__VA_ARGS__)
#define LOG_WARN(format_str, ...) LOG(WARN, format_str, ##__VA_ARGS__)
#define LOG_ERROR(format_str, ...) LOG(ERROR, format_str, ##__VA_ARGS__)
#define LOG_INFO(format_str, ...) LOG(INFO, format_str, ##__VA_ARGS__)
#define LOG(log_level, format_str, ...)                                        \
  do {                                                                         \
    if (log_level >= LOG_LEVEL) {                                              \
      log_message("[%s %s:%d] " format_str "\n",                               \
                  log_level_to_string(log_level), __FILE__, __LINE__,          \
                  ##__VA_ARGS__);                                              \
    }                                                                          \
  } while (0)

void log_message(const char *format_str, ...);
const char *log_level_to_string(LogLevel log_level);

#endif // CUTTERENG_LOG_H
