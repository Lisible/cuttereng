#ifndef CUTTERENG_GRAPHICS_LIGHT_H
#define CUTTERENG_GRAPHICS_LIGHT_H

#include "../math/vector.h"

typedef enum {
  LightType_Directional,
} LightType;

typedef struct {
  v3f position;
} DirectionalLight;

typedef struct {
  LightType type;
  union {
    DirectionalLight directional_light;
  };
} Light;

#endif // CUTTERENG_GRAPHICS_LIGHT_H
