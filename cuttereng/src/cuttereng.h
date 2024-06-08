#ifndef CUTTERENG_H
#define CUTTERENG_H

#include "ecs/ecs.h"

#ifdef CUTTERENG_PLAYDATE_BACKEND
#include "pd_api.h"
typedef struct {
  PlaydateAPI *pd_api;
} PlaydateComponent;
int cuttereng_pd_event_handler(PlaydateAPI *pd, PDSystemEvent event,
                               uint32_t arg, EcsSystemFn ecs_init_system);
#endif

#ifdef CUTTERENG_SDL_BACKEND
void cutter_bootstrap(EcsSystemFn init_system);
#endif

#endif // CUTTERENG_H
