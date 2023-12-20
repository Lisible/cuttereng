#ifndef CUTTERENG_RENDERER_TRANSFORM_H
#define CUTTERENG_RENDERER_TRANSFORM_H

#include "math/quaternion.h"
#include "math/vector.h"

typedef struct {
  v3f position;
  v3f scale;
  Quaternion rotation;
} Transform;

void transform_matrix(const Transform *transform, mat4 transform_matrix);

#define TRANSFORM_DEFAULT                                                      \
  (Transform) {                                                                \
    .position = {0.0, 0.0, 0.0}, .scale = {1.0, 1.0, 1.0},                     \
    .rotation = (Quaternion) {                                                 \
      .scalar_part = 1.0, .vector_part = (v3f) { 0 }                           \
    }                                                                          \
  }

#endif // CUTTERENG_RENDERER_TRANSFORM_H
