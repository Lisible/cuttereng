#include "log.h"
#include "test.h"
#include <ecs/ecs.h>
#include <memory.h>

typedef struct {
  int x;
  int y;
} Position;

typedef struct {
  int x;
  int y;
} Velocity;

void t_ecs_init(void) {
  Ecs ecs;
  ecs_init(&system_allocator, &ecs, ecs_default_init_system);
  ASSERT_EQ(ecs_get_entity_count(&ecs), 0);
  ecs_deinit(&ecs);
}

void t_ecs_create_entity(void) {
  Ecs ecs;
  ecs_init(&system_allocator, &ecs, ecs_default_init_system);
  EcsId id = ecs_create_entity(&ecs);
  ASSERT_EQ(ecs_get_entity_count(&ecs), 1);
  ASSERT_EQ(id, 0);
  EcsId second_id = ecs_create_entity(&ecs);
  ASSERT_EQ(second_id, 1);
  ASSERT_EQ(ecs_get_entity_count(&ecs), 2);
  ecs_deinit(&ecs);
}

void t_ecs_insert_component(void) {
  Ecs ecs;
  ecs_init(&system_allocator, &ecs, ecs_default_init_system);
  EcsId entity = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity, Position, {.x = 4, .y = 22});
  ASSERT(ecs_has_component(&ecs, entity, Position));
  ASSERT(!ecs_has_component(&ecs, entity, Velocity));
  ecs_deinit(&ecs);
}

void t_ecs_get_component(void) {
  Ecs ecs;
  ecs_init(&system_allocator, &ecs, ecs_default_init_system);
  EcsId entity = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity, Position, {.x = 4, .y = 22});
  EcsId entity2 = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity2, Position, {.x = 5, .y = 85});
  ecs_insert_component(&ecs, entity2, Velocity, {.x = 123, .y = 865});

  Position *pos = ecs_get_component(&ecs, entity, Position);
  ASSERT_EQ(pos->x, 4);
  ASSERT_EQ(pos->y, 22);
  Velocity *vel = ecs_get_component(&ecs, entity, Velocity);
  ASSERT_NULL(vel);

  Position *pos2 = ecs_get_component(&ecs, entity2, Position);
  ASSERT_EQ(pos2->x, 5);
  ASSERT_EQ(pos2->y, 85);

  Velocity *vel2 = ecs_get_component(&ecs, entity2, Velocity);
  ASSERT_EQ(vel2->x, 123);
  ASSERT_EQ(vel2->y, 865);
  ecs_deinit(&ecs);
}

void t_ecs_count_matching(void) {
  Ecs ecs;
  ecs_init(&system_allocator, &ecs, ecs_default_init_system);
  EcsId entity = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity, Position, {.x = 4, .y = 22});
  EcsId entity2 = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity2, Position, {.x = 5, .y = 85});
  ecs_insert_component(&ecs, entity2, Velocity, {.x = 123, .y = 865});

  EcsQuery query_positions = {.components = {ecs_component_id(Position)},
                              .component_count = 1};
  ASSERT_EQ(ecs_count_matching(&ecs, &query_positions), 2);
  EcsQuery query_velocities = {.components = {ecs_component_id(Velocity)},
                               .component_count = 1};
  ASSERT_EQ(ecs_count_matching(&ecs, &query_velocities), 1);
  EcsQuery query_positions_and_velocities = {
      .components = {ecs_component_id(Position), ecs_component_id(Velocity)},
      .component_count = 2};
  ASSERT_EQ(ecs_count_matching(&ecs, &query_positions_and_velocities), 1);
  ecs_deinit(&ecs);
}

void t_ecs_query(void) {
  Ecs ecs;
  ecs_init(&system_allocator, &ecs, ecs_default_init_system);
  EcsId entity = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity, Position, {.x = 4, .y = 22});
  EcsId entity2 = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity2, Position, {.x = 5, .y = 85});
  ecs_insert_component(&ecs, entity2, Velocity, {.x = 123, .y = 865});

  EcsQuery query_positions = {.components = {ecs_component_id(Position)},
                              .component_count = 1};
  EcsQueryIt query_iterator = ecs_query(&ecs, &query_positions);

  ecs_query_it_next(&query_iterator);
  Position *pos = ecs_query_it_get(&query_iterator, Position, 0);
  ASSERT_EQ(pos->x, 4);
  ASSERT_EQ(pos->y, 22);

  ecs_query_it_next(&query_iterator);
  pos = ecs_query_it_get(&query_iterator, Position, 0);
  ASSERT_EQ(pos->x, 5);
  ASSERT_EQ(pos->y, 85);
  ASSERT_NULL(ecs_query_it_get(&query_iterator, Position, 1));
  ecs_query_it_deinit(&query_iterator);
  ecs_deinit(&ecs);
}

void t_ecs_query_two_components(void) {
  Ecs ecs;
  ecs_init(&system_allocator, &ecs, ecs_default_init_system);
  EcsId entity = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity, Position, {.x = 4, .y = 22});
  ecs_insert_component(&ecs, entity, Velocity, {.x = 123, .y = 865});
  EcsId entity2 = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity2, Position, {.x = 5, .y = 85});
  EcsId entity3 = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity3, Position, {.x = 9, .y = 5});
  ecs_insert_component(&ecs, entity3, Velocity, {.x = 121, .y = 300});

  EcsQuery query_positions_and_velocities = {
      .components = {ecs_component_id(Position), ecs_component_id(Velocity)},
      .component_count = 2};
  EcsQueryIt query_iterator = ecs_query(&ecs, &query_positions_and_velocities);
  ASSERT_EQ(ecs_count_matching(&ecs, &query_positions_and_velocities), 2);

  ecs_query_it_next(&query_iterator);
  Position *pos = ecs_query_it_get(&query_iterator, Position, 0);
  ASSERT_EQ(pos->x, 4);
  ASSERT_EQ(pos->y, 22);
  Velocity *vel = ecs_query_it_get(&query_iterator, Velocity, 1);
  ASSERT_EQ(vel->x, 123);
  ASSERT_EQ(vel->y, 865);

  ecs_query_it_next(&query_iterator);
  pos = ecs_query_it_get(&query_iterator, Position, 0);
  ASSERT_EQ(pos->x, 9);
  ASSERT_EQ(pos->y, 5);
  vel = ecs_query_it_get(&query_iterator, Velocity, 1);
  ASSERT_EQ(vel->x, 121);
  ASSERT_EQ(vel->y, 300);
  ecs_query_it_deinit(&query_iterator);
  ecs_deinit(&ecs);
}

void print_position_system(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  LOG_DEBUG("Running print_position_system");
  while (ecs_query_it_next(it)) {
    Position *position = ecs_query_it_get(it, Position, 0);
    LOG_INFO("Position: (%d, %d)", position->x, position->y);
  }
}

void move_right_system(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  LOG_DEBUG("Running move_right_system");
  while (ecs_query_it_next(it)) {
    Position *position = ecs_query_it_get(it, Position, 0);
    position->x++;
  }
}

void t_ecs_register_system(void) {
  Ecs ecs;
  ecs_init(&system_allocator, &ecs, ecs_default_init_system);
  EcsId entity = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity, Position, {.x = 4, .y = 22});
  ecs_insert_component(&ecs, entity, Velocity, {.x = 123, .y = 865});
  EcsId entity2 = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity2, Position, {.x = 5, .y = 85});
  EcsId entity3 = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity3, Position, {.x = 9, .y = 5});
  ecs_insert_component(&ecs, entity3, Velocity, {.x = 121, .y = 300});
  ecs_register_system(&ecs, &(const EcsSystemDescriptor){
                                .query = {.components =
                                              {
                                                  ecs_component_id(Position),
                                              },
                                          .component_count = 1},
                                .fn = print_position_system,
                            });
  ecs_register_system(&ecs,
                      &(const EcsSystemDescriptor){
                          .query = {.components = {ecs_component_id(Position)},
                                    .component_count = 1},
                          .fn = move_right_system,
                      });
  ecs_register_system(&ecs,
                      &(const EcsSystemDescriptor){
                          .query = {.components = {ecs_component_id(Position)},
                                    .component_count = 1},
                          .fn = print_position_system,
                      });
  ecs_run_systems(&ecs);
  ecs_deinit(&ecs);
}

TEST_SUITE(TEST(t_ecs_init), TEST(t_ecs_create_entity),
           TEST(t_ecs_insert_component), TEST(t_ecs_get_component),
           TEST(t_ecs_count_matching), TEST(t_ecs_query),
           TEST(t_ecs_query_two_components), TEST(t_ecs_register_system))
