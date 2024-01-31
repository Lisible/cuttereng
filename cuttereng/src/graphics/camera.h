#ifndef CUTTERENG_CAMERA_H
#define CUTTERENG_CAMERA_H

#include "../math/matrix.h"

typedef struct {
  mat4 projection_matrix;
} Camera;

void camera_init_perspective(Camera *camera, float fov_y_deg, float aspect,
                             float near, float far);
mat4_value_type *camera_get_projection_matrix(const Camera *camera);

#endif // CUTTERENG_CAMERA_H
