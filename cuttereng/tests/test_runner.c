#include "test.h"

#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  for (size_t i = 0; i < test_count; i++) {
    fprintf(stderr, "Running test: %s...\n", tests[i].name);
    tests[i].fn();
    fprintf(stderr, ANSI_COLOR_GREEN " OK" ANSI_COLOR_RESET "\n");
  }

  return 0;
}
