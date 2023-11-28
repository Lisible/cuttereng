#include "memory.h"
#include <stdlib.h>

void *memory_allocate(size_t size) { return malloc(size); }
void *memory_allocate_array(size_t count, size_t item_size) {
  return calloc(count, item_size);
}
void *memory_reallocate(void *ptr, size_t new_size) {
  return realloc(ptr, new_size);
}
void memory_free(void *ptr) { free(ptr); }
