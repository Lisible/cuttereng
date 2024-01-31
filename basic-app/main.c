#include <cuttereng.h>
#include <ecs/ecs.h>
#include <engine.h>
#include <graphics/shape.h>
#include <transform.h>

void system_move_cubes(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;

  const SystemContext *system_context = it->ctx;
  while (ecs_query_it_next(it)) {
    Transform *transform = ecs_query_it_get(it, Transform, 0);
    transform->position.x = sin(system_context->current_time_secs);
  }
}

void system_print_cube_infos(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  while (ecs_query_it_next(it)) {
    EcsId entity_id = ecs_query_it_entity_id(it);
    Transform *transform = ecs_query_it_get(it, Transform, 0);
    LOG_DEBUG("Entity %d has position (%f, %f, %f)", entity_id,
              transform->position.x, transform->position.y,
              transform->position.z);
    LOG_DEBUG("Entity %d has cube", entity_id);
  }
}

void init_system(EcsCommandQueue *command_queue) {
  LOG_DEBUG("Initializing");
  ecs_command_queue_register_system(
      command_queue,
      &(const EcsSystemDescriptor){
          .query =
              (EcsQueryDescriptor){.components = {ecs_component_id(Transform),
                                                  ecs_component_id(Cube)},
                                   .component_count = 2},
          .fn = system_move_cubes});
  ecs_command_queue_register_system(
      command_queue,
      &(const EcsSystemDescriptor){
          .query =
              (EcsQueryDescriptor){.components = {ecs_component_id(Transform),
                                                  ecs_component_id(Cube)},
                                   .component_count = 2},
          .fn = system_print_cube_infos});
  EcsId first_entity = ecs_command_queue_create_entity(command_queue);
  Transform first_entity_transform = TRANSFORM_DEFAULT;
  first_entity_transform.position.z = 10.0;
  first_entity_transform.position.x = -4.0;
  first_entity_transform.position.y = -2.0;
  ecs_command_queue_insert_component_with_ptr(
      command_queue, first_entity, Transform, &first_entity_transform);
  ecs_command_queue_insert_tag_component(command_queue, first_entity, Cube);

  EcsId second_entity = ecs_command_queue_create_entity(command_queue);
  Transform second_entity_transform = TRANSFORM_DEFAULT;
  second_entity_transform.position.x = 1.0;
  second_entity_transform.position.y = 0.1;
  second_entity_transform.position.z = 3.0;
  ecs_command_queue_insert_component_with_ptr(
      command_queue, second_entity, Transform, &second_entity_transform);
  ecs_command_queue_insert_tag_component(command_queue, second_entity, Cube);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  cutter_bootstrap(init_system);
  return 0;
}
