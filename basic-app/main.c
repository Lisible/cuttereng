#include <cuttereng.h>
#include <ecs/ecs.h>
#include <engine.h>
#include <input.h>
#include <math/quaternion.h>
#include <math/vector.h>
#include <transform.h>

void init_system(EcsCommandQueue *command_queue, EcsQueryIt *it) {
  LOG_DEBUG("Initializing");
  (void)it;
  (void)command_queue;
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  cutter_bootstrap(init_system);
  return 0;
}
