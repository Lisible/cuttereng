#include "log.h"
#include "test.h"
#include <ecs.h>

typedef struct {
  int x;
  int y;
} Position;

typedef struct {
  int x;
  int y;
} Velocity;

void t_ecs_init() {
  Ecs ecs;
  ecs_init(&ecs);
  ASSERT_EQ(ecs_get_entity_count(&ecs), 0);
  ecs_deinit(&ecs);
}

void t_ecs_create_entity() {
  Ecs ecs;
  ecs_init(&ecs);
  EcsId id = ecs_create_entity(&ecs);
  ASSERT_EQ(ecs_get_entity_count(&ecs), 1);
  ASSERT_EQ(id, 0);
  EcsId second_id = ecs_create_entity(&ecs);
  ASSERT_EQ(second_id, 1);
  ASSERT_EQ(ecs_get_entity_count(&ecs), 2);
  ecs_deinit(&ecs);
}

void t_ecs_insert_component() {
  Ecs ecs;
  ecs_init(&ecs);
  EcsId entity = ecs_create_entity(&ecs);
  ecs_insert(&ecs, entity, Position, {.x = 4, .y = 22});
  ASSERT(ecs_has(&ecs, entity, Position));
  ASSERT(!ecs_has(&ecs, entity, Velocity));
  ecs_deinit(&ecs);
}

void t_ecs_get_component() {
  Ecs ecs;
  ecs_init(&ecs);
  EcsId entity = ecs_create_entity(&ecs);
  ecs_insert(&ecs, entity, Position, {.x = 4, .y = 22});
  EcsId entity2 = ecs_create_entity(&ecs);
  ecs_insert(&ecs, entity2, Position, {.x = 5, .y = 85});
  ecs_insert(&ecs, entity2, Velocity, {.x = 123, .y = 865});

  Position *pos = ecs_get(&ecs, entity, Position);
  ASSERT_EQ(pos->x, 4);
  ASSERT_EQ(pos->y, 22);
  Velocity *vel = ecs_get(&ecs, entity, Velocity);
  ASSERT_NULL(vel);

  Position *pos2 = ecs_get(&ecs, entity2, Position);
  ASSERT_EQ(pos2->x, 5);
  ASSERT_EQ(pos2->y, 85);

  Velocity *vel2 = ecs_get(&ecs, entity2, Velocity);
  ASSERT_EQ(vel2->x, 123);
  ASSERT_EQ(vel2->y, 865);
  ecs_deinit(&ecs);
}

void t_ecs_count_matching() {
  Ecs ecs;
  ecs_init(&ecs);
  EcsId entity = ecs_create_entity(&ecs);
  ecs_insert(&ecs, entity, Position, {.x = 4, .y = 22});
  EcsId entity2 = ecs_create_entity(&ecs);
  ecs_insert(&ecs, entity2, Position, {.x = 5, .y = 85});
  ecs_insert(&ecs, entity2, Velocity, {.x = 123, .y = 865});

  EcsQuery query_positions = {.components =
                                  (char *[]){ecs_component_id(Position), NULL}};
  ASSERT_EQ(ecs_count_matching(&ecs, &query_positions), 2);
  EcsQuery query_velocities = {
      .components = (char *[]){ecs_component_id(Velocity), NULL}};
  ASSERT_EQ(ecs_count_matching(&ecs, &query_velocities), 1);
  EcsQuery query_positions_and_velocities = {
      .components = (char *[]){ecs_component_id(Position),
                               ecs_component_id(Velocity), NULL}};
  ASSERT_EQ(ecs_count_matching(&ecs, &query_positions_and_velocities), 1);
}

TEST_SUITE(TEST(t_ecs_init), TEST(t_ecs_create_entity),
           TEST(t_ecs_insert_component), TEST(t_ecs_get_component),
           TEST(t_ecs_count_matching));
