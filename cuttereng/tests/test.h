#ifndef TEST_H
#define TEST_H

#include <math.h> // IWYU pragma: keep
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_DATA_PATH "../cuttereng/tests/tests_data/"

typedef void (*TestFn)(void);

typedef struct {
  const char *name;
  TestFn fn;
} Test;

extern Test tests[];
extern size_t test_count;

inline static void assert(int boolean, const char *fmt, ...) {
  if (!boolean) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, fmt, args);
    va_end(args);
    exit(1);
  }
}

#define T_ASSERT(expr) T_ASSERT_MSG(expr, #expr)

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define T_ASSERT_MSG(expr, fmt, ...)                                           \
  do {                                                                         \
    assert((expr),                                                             \
           "Assertion failed in " __FILE__ ":" TOSTRING(__LINE__) ": " fmt     \
                                                                  "\n",        \
           ##__VA_ARGS__);                                                     \
  } while (0)

#define T_ASSERT_EQ(a, b) T_ASSERT_MSG((a == b), #a " != " #b)
#define T_ASSERT_FLOAT_EQ(a, b, epsilon)                                       \
  T_ASSERT_MSG((fabs(a - b) < epsilon), #a " != " #b)

#define T_ASSERT_NULL(a) T_ASSERT_MSG((a == NULL), #a " is not NULL")
#define T_ASSERT_NOT_NULL(a) T_ASSERT_MSG((a != NULL), #a " is NULL")

#define T_ASSERT_STR_EQ(a, b, length)                                          \
  T_ASSERT_MSG((strncmp(a, b, length) == 0), #a " != " #b)

#define ARG_COUNT(...) (sizeof((Test[]){__VA_ARGS__}) / sizeof(Test))
#define TEST_SUITE(...)                                                        \
  Test tests[] = {__VA_ARGS__};                                                \
  size_t test_count = ARG_COUNT(__VA_ARGS__);
#define TEST(x)                                                                \
  { .name = #x, .fn = x }

#endif // TEST_H
