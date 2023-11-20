#include "log.h"
#include <stdarg.h>
#include <stdio.h>

void log_message(log_level_t log_level, const char *filename, int line_number,
                 const char *format_str, ...) {
  va_list args;
  va_start(args, format_str);
  fprintf(stderr, format_str, log_level_to_string(log_level), filename,
          line_number, args);
  va_end(args);
}

const char *log_level_to_string(log_level_t log_level) {
  switch (log_level) {
  case TRACE:
    return "TRACE";
  case DEBUG:
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
