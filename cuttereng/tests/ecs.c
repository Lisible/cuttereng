#include "test.h"
#include <ecs.h>

typedef struct position {
  int x;
  int y;
} position;

typedef struct velocity {
  int x;
  int y;
} velocity;

void t_ecs_init() {
  ecs ecs;
  ecs_init(&ecs);
  ASSERT_EQ(ecs_get_entity_count(&ecs), 0);
}

void t_ecs_create_entity() {
  ecs ecs;
  ecs_init(&ecs);
  ecs_id id = ecs_create_entity(&ecs);
  ASSERT_EQ(ecs_get_entity_count(&ecs), 1);
  ASSERT_EQ(id, 0);
  ecs_id second_id = ecs_create_entity(&ecs);
  ASSERT_EQ(second_id, 1);
  ASSERT_EQ(ecs_get_entity_count(&ecs), 2);
}

void t_ecs_insert_component() {
  ecs ecs;
  ecs_init(&ecs);
  ecs_id entity = ecs_create_entity(&ecs);
  ecs_insert(&ecs, entity, position, {.x = 4, .y = 22});
  ASSERT(ecs_has(&ecs, entity, position));
  ASSERT(!ecs_has(&ecs, entity, velocity));
}

TEST_SUITE(TEST(t_ecs_init), TEST(t_ecs_create_entity),
           TEST(t_ecs_insert_component));
