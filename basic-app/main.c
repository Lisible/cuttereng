#include <cuttereng.h>

void init_system(EcsCommandQueue *command_queue, EcsQueryIt *it) {
  (void)command_queue;
}

#ifdef CUTTERENG_PLAYDATE_BACKEND
#include <pd_api.h>
int eventHandler(PlaydateAPI *pd, PDSystemEvent event, uint32_t arg) {
  return cuttereng_pd_event_handler(pd, event, arg, init_system);
}
#endif

#ifdef CUTTERENG_SDL_BACKEND
int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  cutter_bootstrap(init_system);
  return 0;
}
#endif
