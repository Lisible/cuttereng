#ifndef TEST_H
#define TEST_H

#include <math.h>
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

#define ASSERT(expr) ASSERT_MSG(expr, #expr)

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define ASSERT_MSG(expr, fmt, ...)                                             \
  do {                                                                         \
    assert((expr),                                                             \
           "Assertion failed in " __FILE__ ":" TOSTRING(__LINE__) ": " fmt     \
                                                                  "\n",        \
           ##__VA_ARGS__);                                                     \
  } while (0)

#define ASSERT_EQ(a, b) ASSERT_MSG((a == b), #a " != " #b)
#define ASSERT_FLOAT_EQ(a, b, epsilon)                                         \
  ASSERT_MSG((fabs(a - b) < epsilon), #a " != " #b)

#define ASSERT_NULL(a) ASSERT_MSG((a == NULL), #a " is not NULL")
#define ASSERT_NOT_NULL(a) ASSERT_MSG((a != NULL), #a " is NULL")

#define ASSERT_STR_EQ(a, b, length)                                            \
  ASSERT_MSG((strncmp(a, b, length) == 0), #a " != " #b)

#define ARG_COUNT(...) (sizeof((Test[]){__VA_ARGS__}) / sizeof(Test))
#define TEST_SUITE(...)                                                        \
  Test tests[] = {__VA_ARGS__};                                                \
  size_t test_count = ARG_COUNT(__VA_ARGS__);
#define TEST(x)                                                                \
  (Test) { .name = #x, .fn = x }

#endif // TEST_H
