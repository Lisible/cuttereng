#include "fs.h"
#include <stddef.h>

void fs_path_pop(char *const inout_path) {
  size_t index_of_last_path_separator = 0;
  size_t i = 0;
  while (inout_path[i] != '\0') {
    if (inout_path[i] == '/') {
      index_of_last_path_separator = i;
    }
    i++;
  }

  inout_path[index_of_last_path_separator + 1] = '\0';
}
