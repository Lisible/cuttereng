#include "camera.h"
#include "assert.h"
#include "math/matrix.h"
#include "memory.h"

void camera_init_perspective(Camera *camera, float fov_y_deg, float aspect,
                             float near, float far) {
  ASSERT(camera != NULL);
  mat4_set_to_perspective(camera->projection_matrix, fov_y_deg, aspect, near,
                          far);
}

mat4_value_type *camera_get_projection_matrix(const Camera *camera) {
  ASSERT(camera != NULL);
  return (mat4_value_type *)camera->projection_matrix;
}
