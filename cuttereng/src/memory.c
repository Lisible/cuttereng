#include "memory.h"
#include "assert.h"
#include <stdlib.h>

void *memory_allocate(size_t size, void *ctx) { return malloc(size); }
void *memory_allocate_array(size_t count, size_t item_size, void *ctx) {
  return calloc(count, item_size);
}
void *memory_reallocate(void *ptr, size_t new_size, void *ctx) {
  return realloc(ptr, new_size);
}
void memory_free(void *ptr, void *ctx) { free(ptr); }
Allocator system_allocator = {.allocate = memory_allocate,
                              .allocate_array = memory_allocate_array,
                              .reallocate = memory_reallocate,
                              .free = memory_free,
                              NULL};
void *allocator_allocate(Allocator *allocator, size_t size) {
  ASSERT(allocator != NULL);
  return allocator->allocate(size, allocator->ctx);
}
void *allocator_allocate_array(Allocator *allocator, size_t count,
                               size_t item_size) {
  ASSERT(allocator != NULL);
  return allocator->allocate_array(count, item_size, allocator->ctx);
}
void *allocator_reallocate(Allocator *allocator, void *ptr, size_t new_size) {
  ASSERT(allocator != NULL);
  return allocator->reallocate(ptr, new_size, allocator->ctx);
}
void allocator_free(Allocator *allocator, void *ptr) {
  ASSERT(allocator != NULL);
  allocator->free(ptr, allocator->ctx);
}
