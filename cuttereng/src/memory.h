#ifndef CUTTERENG_MEMORY_H
#define CUTTERENG_MEMORY_H

#include "common.h"
/// @file

typedef struct {
  void *(*allocate)(size_t size, void *ctx);
  void *(*allocate_aligned)(size_t alignment, size_t size, void *ctx);
  void *(*allocate_array)(size_t count, size_t item_size, void *ctx);
  void *(*reallocate)(void *ptr, size_t old_size, size_t new_size, void *ctx);
  void (*free)(void *ptr, void *ctx);
  void *ctx;
} Allocator;

void *allocator_allocate(Allocator *allocator, size_t size);
void *allocator_allocate_aligned(Allocator *allocator, size_t alignment,
                                 size_t size);
void *allocator_allocate_array(Allocator *allocator, size_t count,
                               size_t item_size);
void *allocator_reallocate(Allocator *allocator, void *ptr, size_t old_size,
                           size_t new_size);
void allocator_free(Allocator *allocator, void *ptr);

extern Allocator system_allocator;

typedef struct {
  size_t capacity;
  size_t size;
  u8 *data;
} Arena;

Allocator arena_allocator(Arena *arena);
void arena_init(Arena *arena, Allocator *allocator, size_t size);
void *arena_allocate(Arena *arena, size_t size);
void *arena_allocate_array(Arena *arena, size_t count, size_t item_size);
void arena_clear(Arena *arena);
void arena_deinit(Arena *arena, Allocator *allocator);

char *memory_clone_string(Allocator *allocator, const char *str);

#endif
