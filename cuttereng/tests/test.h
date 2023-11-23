#ifndef TEST_H
#define TEST_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*test_fn)(void);

struct test {
  char *name;
  test_fn fn;
};
typedef struct test test;

extern test tests[];
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

#define ASSERT_STR_EQ(a, b, length)                                            \
  ASSERT_MSG((strncmp(a, b, length) == 0), #a " != " #b)

#define ARG_COUNT(...) (sizeof((test[]){__VA_ARGS__}) / sizeof(test))
#define TEST_SUITE(...)                                                        \
  test tests[] = {__VA_ARGS__};                                                \
  size_t test_count = ARG_COUNT(__VA_ARGS__);
#define TEST(x)                                                                \
  (test) { .name = #x, .fn = x }

#endif // TEST_H
