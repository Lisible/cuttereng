#include "memory.h"
#include "assert.h"
#include "src/common.h"
#include <stdlib.h>
#include <string.h>

void *memory_allocate(size_t size, void *ctx) {
  (void)ctx;
  return malloc(size);
}
void *memory_allocate_array(size_t count, size_t item_size, void *ctx) {
  (void)ctx;
  return calloc(count, item_size);
}
void *memory_reallocate(void *ptr, size_t old_size, size_t new_size,
                        void *ctx) {
  (void)old_size;
  (void)ctx;
  return realloc(ptr, new_size);
}
void memory_free(void *ptr, void *ctx) {
  (void)ctx;
  free(ptr);
}
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
void *allocator_reallocate(Allocator *allocator, void *ptr, size_t old_size,
                           size_t new_size) {
  ASSERT(allocator != NULL);
  return allocator->reallocate(ptr, old_size, new_size, allocator->ctx);
}
void allocator_free(Allocator *allocator, void *ptr) {
  ASSERT(allocator != NULL);
  allocator->free(ptr, allocator->ctx);
}

void *arena_allocator_allocate(size_t size, void *ctx) {
  ASSERT(ctx != NULL);
  return arena_allocate((Arena *)ctx, size);
}
void *arena_allocator_allocate_array(size_t count, size_t item_size,
                                     void *ctx) {
  ASSERT(ctx != NULL);
  return arena_allocate_array((Arena *)ctx, count, item_size);
}
void *arena_allocator_reallocate(void *ptr, size_t old_size, size_t new_size,
                                 void *ctx) {
  (void)ptr;
  (void)old_size;
  (void)new_size;
  (void)ctx;
  UNIMPLEMENTED();
}

void arena_allocator_free(void *ptr, void *ctx) {
  (void)ptr;
  (void)ctx;
}

Allocator arena_allocator(Arena *arena) {
  return (Allocator){.ctx = arena,
                     .allocate = arena_allocator_allocate,
                     .allocate_array = arena_allocator_allocate_array,
                     .reallocate = arena_allocator_reallocate,
                     .free = arena_allocator_free};
}

void arena_init(Arena *arena, Allocator *allocator, size_t size) {
  ASSERT(arena != NULL);
  ASSERT(allocator != NULL);
  arena->size = 0;
  arena->capacity = size;
  arena->data = allocator_allocate(allocator, size);
}
void *arena_allocate(Arena *arena, size_t size) {
  ASSERT(arena->size + size <= arena->capacity);
  void *ptr = arena->data + arena->size;
  arena->size += size;
  return ptr;
}
void *arena_allocate_array(Arena *arena, size_t count, size_t item_size) {
  void *ptr = arena_allocate(arena, count * item_size);
  memset(ptr, 0, count * item_size);
  return ptr;
}
void *arena_reallocate(Arena *arena, void *ptr, size_t old_size,
                       size_t new_size) {
  ASSERT(new_size > old_size);
  void *new_ptr = arena_allocate(arena, new_size);
  memcpy(new_ptr, ptr, old_size);
  return new_ptr;
}
void arena_clear(Arena *arena) { arena->size = 0; }
void arena_deinit(Arena *arena, Allocator *allocator) {
  allocator_free(allocator, arena->data);
}

char *memory_clone_string(Allocator *allocator, char *str) {
  size_t string_length = strlen(str) + 1;
  char *duplicate_string =
      allocator_allocate_array(allocator, string_length, sizeof(char));
  memcpy(duplicate_string, str, string_length);
  return duplicate_string;
}
