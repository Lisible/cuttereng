#include "log.h"
#include <stdarg.h>
#include <stdio.h>

void log_message(const char *format_str, ...) {
  va_list args;
  va_start(args, format_str);
  vfprintf(stderr, format_str, args);
  va_end(args);
}

const char *log_level_to_string(log_level_t log_level) {
  switch (log_level) {
  case TRACE:
    return "TRACE";
  case DBG:
    return "DEBUG";
  case WARN:
    return "WARN";
  case ERROR:
    return "ERROR";
  case INFO:
  default:
    return "INFO";
    break;
  }
}
