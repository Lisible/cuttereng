#include "test.h"
#include <ecs/ecs.h>
#include <lisiblestd/log.h>
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
  ecs_init(&system_allocator, &ecs, ecs_default_init_system, NULL);
  T_ASSERT_EQ(ecs_get_entity_count(&ecs), 0);
  ecs_deinit(&ecs);
}

void t_ecs_create_entity(void) {
  Ecs ecs;
  ecs_init(&system_allocator, &ecs, ecs_default_init_system, NULL);
  EcsId id = ecs_create_entity(&ecs);
  T_ASSERT_EQ(ecs_get_entity_count(&ecs), 1);
  T_ASSERT_EQ(id, 0);
  EcsId second_id = ecs_create_entity(&ecs);
  T_ASSERT_EQ(second_id, 1);
  T_ASSERT_EQ(ecs_get_entity_count(&ecs), 2);
  ecs_deinit(&ecs);
}

void t_ecs_insert_component(void) {
  Ecs ecs;
  ecs_init(&system_allocator, &ecs, ecs_default_init_system, NULL);
  EcsId entity = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity, Position, {.x = 4, .y = 22});
  T_ASSERT(ecs_has_component(&ecs, entity, Position));
  T_ASSERT(!ecs_has_component(&ecs, entity, Velocity));
  ecs_deinit(&ecs);
}

void t_ecs_insert_relationship(void) {
  Ecs ecs;
  ecs_init(&system_allocator, &ecs, ecs_default_init_system, NULL);
  EcsId entity = ecs_create_entity(&ecs);
  EcsId child_entity = ecs_create_entity(&ecs);
  EcsId second_child_entity = ecs_create_entity(&ecs);
  ecs_insert_relationship(&ecs, entity, ChildOf, child_entity);
  ecs_insert_relationship(&ecs, entity, ChildOf, second_child_entity);

  HashSet *sources = ecs_get_relationship_sources(&ecs, ChildOf, child_entity);
  T_ASSERT_EQ(HashSet_length(sources), 1);
  T_ASSERT(HashSet_has(sources, &entity));

  HashSet *targets = ecs_get_relationship_targets(&ecs, entity, ChildOf);
  T_ASSERT_EQ(HashSet_length(targets), 2);
  T_ASSERT(HashSet_has(targets, &child_entity));
  T_ASSERT(HashSet_has(targets, &second_child_entity));
  ecs_deinit(&ecs);
}

void t_ecs_get_component(void) {
  Ecs ecs;
  ecs_init(&system_allocator, &ecs, ecs_default_init_system, NULL);
  EcsId entity = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity, Position, {.x = 4, .y = 22});
  EcsId entity2 = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity2, Position, {.x = 5, .y = 85});
  ecs_insert_component(&ecs, entity2, Velocity, {.x = 123, .y = 865});

  Position *pos = ecs_get_component(&ecs, entity, Position);
  T_ASSERT_EQ(pos->x, 4);
  T_ASSERT_EQ(pos->y, 22);
  Velocity *vel = ecs_get_component(&ecs, entity, Velocity);
  T_ASSERT_NULL(vel);

  Position *pos2 = ecs_get_component(&ecs, entity2, Position);
  T_ASSERT_EQ(pos2->x, 5);
  T_ASSERT_EQ(pos2->y, 85);

  Velocity *vel2 = ecs_get_component(&ecs, entity2, Velocity);
  T_ASSERT_EQ(vel2->x, 123);
  T_ASSERT_EQ(vel2->y, 865);
  ecs_deinit(&ecs);
}

void t_ecs_count_matching(void) {
  Ecs ecs;
  ecs_init(&system_allocator, &ecs, ecs_default_init_system, NULL);
  EcsId entity = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity, Position, {.x = 4, .y = 22});
  EcsId entity2 = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity2, Position, {.x = 5, .y = 85});
  ecs_insert_component(&ecs, entity2, Velocity, {.x = 123, .y = 865});

  EcsQueryDescriptor query_positions_descriptor = {
      .components = {ecs_component_id(Position)}, .component_count = 1};
  EcsQuery *query_positions =
      EcsQuery_new(&system_allocator, &query_positions_descriptor);
  T_ASSERT_EQ(ecs_count_matching(&ecs, query_positions), 2);
  EcsQueryDescriptor query_velocities_descriptor = {
      .components = {ecs_component_id(Velocity)}, .component_count = 1};
  EcsQuery *query_velocities =
      EcsQuery_new(&system_allocator, &query_velocities_descriptor);
  T_ASSERT_EQ(ecs_count_matching(&ecs, query_velocities), 1);
  EcsQueryDescriptor query_positions_and_velocities_descriptor = {
      .components = {ecs_component_id(Position), ecs_component_id(Velocity)},
      .component_count = 2};
  EcsQuery *query_positions_and_velocities =
      EcsQuery_new(ecs.allocator, &query_positions_and_velocities_descriptor);
  T_ASSERT_EQ(ecs_count_matching(&ecs, query_positions_and_velocities), 1);

  EcsQuery_destroy(query_positions, ecs.allocator);
  EcsQuery_destroy(query_velocities, ecs.allocator);
  EcsQuery_destroy(query_positions_and_velocities, ecs.allocator);
  ecs_deinit(&ecs);
}

void t_ecs_query(void) {
  Ecs ecs;
  ecs_init(&system_allocator, &ecs, ecs_default_init_system, NULL);
  EcsId entity = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity, Position, {.x = 4, .y = 22});
  EcsId entity2 = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity2, Position, {.x = 5, .y = 85});
  ecs_insert_component(&ecs, entity2, Velocity, {.x = 123, .y = 865});

  EcsQueryDescriptor query_positions_descriptor = {
      .components = {ecs_component_id(Position)}, .component_count = 1};
  EcsQuery *query_positions =
      EcsQuery_new(&system_allocator, &query_positions_descriptor);
  EcsQueryIt query_iterator = ecs_query(&ecs, query_positions);

  ecs_query_it_next(&query_iterator);
  Position *pos = ecs_query_it_get(&query_iterator, Position, 0);
  T_ASSERT_EQ(pos->x, 4);
  T_ASSERT_EQ(pos->y, 22);

  ecs_query_it_next(&query_iterator);
  pos = ecs_query_it_get(&query_iterator, Position, 0);
  T_ASSERT_EQ(pos->x, 5);
  T_ASSERT_EQ(pos->y, 85);
  T_ASSERT_NULL(ecs_query_it_get(&query_iterator, Position, 1));
  ecs_query_it_deinit(&query_iterator);
  EcsQuery_destroy(query_positions, ecs.allocator);
  ecs_deinit(&ecs);
}

void t_ecs_query_two_components(void) {
  Ecs ecs;
  ecs_init(&system_allocator, &ecs, ecs_default_init_system, NULL);
  EcsId entity = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity, Position, {.x = 4, .y = 22});
  ecs_insert_component(&ecs, entity, Velocity, {.x = 123, .y = 865});
  EcsId entity2 = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity2, Position, {.x = 5, .y = 85});
  EcsId entity3 = ecs_create_entity(&ecs);
  ecs_insert_component(&ecs, entity3, Position, {.x = 9, .y = 5});
  ecs_insert_component(&ecs, entity3, Velocity, {.x = 121, .y = 300});

  EcsQueryDescriptor query_positions_and_velocities_descriptor = {
      .components = {ecs_component_id(Position), ecs_component_id(Velocity)},
      .component_count = 2};
  EcsQuery *query_positions_and_velocities =
      EcsQuery_new(ecs.allocator, &query_positions_and_velocities_descriptor);
  EcsQueryIt query_iterator = ecs_query(&ecs, query_positions_and_velocities);
  T_ASSERT_EQ(ecs_count_matching(&ecs, query_positions_and_velocities), 2);

  ecs_query_it_next(&query_iterator);
  Position *pos = ecs_query_it_get(&query_iterator, Position, 0);
  T_ASSERT_EQ(pos->x, 4);
  T_ASSERT_EQ(pos->y, 22);
  Velocity *vel = ecs_query_it_get(&query_iterator, Velocity, 1);
  T_ASSERT_EQ(vel->x, 123);
  T_ASSERT_EQ(vel->y, 865);

  ecs_query_it_next(&query_iterator);
  pos = ecs_query_it_get(&query_iterator, Position, 0);
  T_ASSERT_EQ(pos->x, 9);
  T_ASSERT_EQ(pos->y, 5);
  vel = ecs_query_it_get(&query_iterator, Velocity, 1);
  T_ASSERT_EQ(vel->x, 121);
  T_ASSERT_EQ(vel->y, 300);
  EcsQuery_destroy(query_positions_and_velocities, ecs.allocator);
  ecs_query_it_deinit(&query_iterator);
  ecs_deinit(&ecs);
}

void print_position_system(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  LOG0_DEBUG("Running print_position_system");
  while (ecs_query_it_next(it)) {
    Position *position = ecs_query_it_get(it, Position, 0);
    LOG_INFO("Position: (%d, %d)", position->x, position->y);
  }
}

void move_right_system(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  LOG0_DEBUG("Running move_right_system");
  while (ecs_query_it_next(it)) {
    Position *position = ecs_query_it_get(it, Position, 0);
    position->x++;
  }
}

void t_ecs_register_system(void) {
  Ecs ecs;
  ecs_init(&system_allocator, &ecs, ecs_default_init_system, NULL);
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
  ecs_run_systems(&ecs, NULL);
  ecs_deinit(&ecs);
}

TEST_SUITE(TEST(t_ecs_init), TEST(t_ecs_create_entity),
           TEST(t_ecs_insert_component), TEST(t_ecs_get_component),
           TEST(t_ecs_count_matching), TEST(t_ecs_query),
           TEST(t_ecs_query_two_components), TEST(t_ecs_register_system),
           TEST(t_ecs_insert_relationship))
