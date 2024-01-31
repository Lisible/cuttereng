#include <cuttereng.h>
#include <ecs/ecs.h>
#include <graphics/shape.h>

typedef struct {
  int x;
  int y;
} Position;

void system_a(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  (void)it;
  LOG_DEBUG("A");
}

void system_b(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  while (ecs_query_it_next(it)) {
    EcsId entity_id = ecs_query_it_entity_id(it);
    Position *position = ecs_query_it_get(it, Position, 0);
    LOG_DEBUG("Entity %d has position (%d, %d)", entity_id, position->x,
              position->y);
    LOG_DEBUG("Entity %d has cube", entity_id);
  }
}

void init_system(EcsCommandQueue *command_queue) {
  LOG_DEBUG("Initializing");
  ecs_command_queue_register_system(
      command_queue, &(const EcsSystemDescriptor){
                         .query = (EcsQueryDescriptor){0}, .fn = system_a});
  ecs_command_queue_register_system(
      command_queue,
      &(const EcsSystemDescriptor){
          .query =
              (EcsQueryDescriptor){.components = {ecs_component_id(Position),
                                                  ecs_component_id(Cube)},
                                   .component_count = 2},
          .fn = system_b});
  EcsId first_entity = ecs_command_queue_create_entity(command_queue);
  ecs_command_queue_insert_component(command_queue, first_entity, Position,
                                     {.x = 23, .y = 25});
  // ecs_command_queue_insert_tag_component(command_queue, first_entity, Cube);
  EcsId second_entity = ecs_command_queue_create_entity(command_queue);
  ecs_command_queue_insert_component(command_queue, second_entity, Position,
                                     {.x = 4, .y = 5});
  ecs_command_queue_insert_tag_component(command_queue, second_entity, Cube);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  cutter_bootstrap(init_system);
  return 0;
}
