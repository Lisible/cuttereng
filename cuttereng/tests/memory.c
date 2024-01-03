#include "test.h"
#include <log.h>
#include <memory.h>

void t_arena_create(void) {
  Arena arena;
  arena_init(&arena, &system_allocator, 1024);
  ASSERT_EQ(arena.capacity, 1024);
  ASSERT_EQ(arena.size, 0);
  ASSERT_NOT_NULL(arena.data);
  arena_deinit(&arena, &system_allocator);
}

void t_arena_allocate(void) {
  Arena arena;
  arena_init(&arena, &system_allocator, 10);
  void *ptr = arena_allocate(&arena, 10);
  ASSERT_NOT_NULL(ptr);
  ASSERT_EQ(arena.size, 10);
  ASSERT_EQ(arena.capacity, 10);
  arena_deinit(&arena, &system_allocator);
}

void t_arena_allocate_array(void) {
  Arena arena;
  arena_init(&arena, &system_allocator, 100);
  void *ptr = arena_allocate_array(&arena, 10, sizeof(int));
  ASSERT_NOT_NULL(ptr);
  ASSERT_EQ(arena.size, 10 * sizeof(int));
  ASSERT_EQ(arena.capacity, 100);
  arena_deinit(&arena, &system_allocator);
}

void t_arena_allocate_clear(void) {
  Arena arena;
  arena_init(&arena, &system_allocator, 100);
  arena_allocate(&arena, 8);
  ASSERT_EQ(arena.size, 8);
  ASSERT_EQ(arena.capacity, 100);
  arena_allocate(&arena, 4);
  ASSERT_EQ(arena.size, 12);
  ASSERT_EQ(arena.capacity, 100);
  arena_clear(&arena);
  ASSERT_EQ(arena.size, 0);
  ASSERT_EQ(arena.capacity, 100);
  arena_deinit(&arena, &system_allocator);
}

TEST_SUITE(TEST(t_arena_create), TEST(t_arena_allocate),
           TEST(t_arena_allocate_array), TEST(t_arena_allocate_clear))
