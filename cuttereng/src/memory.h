#ifndef CUTTERENG_MEMORY_H
#define CUTTERENG_MEMORY_H

#include "common.h"
/// @file

typedef struct {
  void *(*allocate)(size_t size, void *ctx);
  void *(*allocate_array)(size_t count, size_t item_size, void *ctx);
  void *(*reallocate)(void *ptr, size_t old_size, size_t new_size, void *ctx);
  void (*free)(void *ptr, void *ctx);
  void *ctx;
} Allocator;

void *allocator_allocate(Allocator *allocator, size_t size);
void *allocator_allocate_array(Allocator *allocator, size_t count,
                               size_t item_size);
void *allocator_reallocate(Allocator *allocator, void *ptr, size_t old_size,
                           size_t new_size);
void allocator_free(Allocator *allocator, void *ptr);

extern Allocator system_allocator;

#endif
