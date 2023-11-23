#include "test.h"

int main(int argc, char **argv) {
  for (size_t i = 0; i < test_count; i++) {
    fprintf(stderr, "Running test: %s...\n", tests[i].name);
    tests[i].fn();
  }

  return 0;
}
