#ifndef CUTTERENG_COMMON_H
#define CUTTERENG_COMMON_H

#include "log.h"
#include <float.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define PANIC(msg, ...)                                                        \
  do {                                                                         \
    LOG_ERROR("Panicked in function %s:%d %s()\n" msg, __FILE__, __LINE__,     \
              __func__, ##__VA_ARGS__);                                        \
    exit(1);                                                                   \
  } while (0)

#define UNIMPLEMENTED(...)                                                     \
  do {                                                                         \
    LOG_ERROR("Reached unimplemented code in function %s:%d %s()\n%s",         \
              __FILE__, __LINE__, __func__, ##__VA_ARGS__);                    \
    exit(1);                                                                   \
  } while (0)

typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef int16_t i16;
typedef uint16_t u16;
typedef int8_t i8;
typedef uint8_t u8;

#endif // CUTTERENG_COMMON_H
