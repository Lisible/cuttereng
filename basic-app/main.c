#include "ecs/ecs.h"
#include <cuttereng.h>

void system_a(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  (void)it;
  LOG_DEBUG("A");
}

void system_b(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  (void)it;
  LOG_DEBUG("B");
}

void init_system(EcsCommandQueue *command_queue) {
  LOG_DEBUG("Initializing");
  ecs_command_queue_register_system(
      command_queue,
      &(const EcsSystemDescriptor){.query = (EcsQuery){0}, .fn = system_a});
  ecs_command_queue_register_system(
      command_queue,
      &(const EcsSystemDescriptor){.query = (EcsQuery){0}, .fn = system_b});
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  cutter_bootstrap(init_system);
  return 0;
}
