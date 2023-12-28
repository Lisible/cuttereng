#ifndef CUTTERENG_ASSERT_H
#define CUTTERENG_ASSERT_H

#include "log.h"
#include <stdlib.h>

#ifdef DEBUG
#define ASSERT(expr, ...)                                                      \
  do {                                                                         \
    if (!(expr)) {                                                             \
      LOG_ERROR("Assertion failed: \n\t%s" #expr, ##__VA_ARGS__);              \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)
#else
#define ASSERT(expr)
#endif // DEBUG

#endif // CUTTERENG_ASSERT_H
