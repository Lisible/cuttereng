#include "log.h"
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
  ecs_deinit(&ecs);
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
  ecs_deinit(&ecs);
}

void t_ecs_insert_component() {
  ecs ecs;
  ecs_init(&ecs);
  ecs_id entity = ecs_create_entity(&ecs);
  ecs_insert(&ecs, entity, position, {.x = 4, .y = 22});
  ASSERT(ecs_has(&ecs, entity, position));
  ASSERT(!ecs_has(&ecs, entity, velocity));
  ecs_deinit(&ecs);
}

void t_ecs_get_component() {
  ecs ecs;
  ecs_init(&ecs);
  ecs_id entity = ecs_create_entity(&ecs);
  ecs_insert(&ecs, entity, position, {.x = 4, .y = 22});
  ecs_id entity2 = ecs_create_entity(&ecs);
  ecs_insert(&ecs, entity2, position, {.x = 5, .y = 85});
  ecs_insert(&ecs, entity2, velocity, {.x = 123, .y = 865});

  position *pos = ecs_get(&ecs, entity, position);
  ASSERT_EQ(pos->x, 4);
  ASSERT_EQ(pos->y, 22);
  velocity *vel = ecs_get(&ecs, entity, velocity);
  ASSERT_NULL(vel);

  position *pos2 = ecs_get(&ecs, entity2, position);
  ASSERT_EQ(pos2->x, 5);
  ASSERT_EQ(pos2->y, 85);

  velocity *vel2 = ecs_get(&ecs, entity2, velocity);
  ASSERT_EQ(vel2->x, 123);
  ASSERT_EQ(vel2->y, 865);
  ecs_deinit(&ecs);
}

TEST_SUITE(TEST(t_ecs_init), TEST(t_ecs_create_entity),
           TEST(t_ecs_insert_component), TEST(t_ecs_get_component));
