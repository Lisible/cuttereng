#include "camera.h"
#include "src/math/matrix.h"
#include "src/memory.h"

Camera *camera_new_perspective(float fov_y_deg, float aspect, float near,
                               float far) {
  Camera *camera = memory_allocate(sizeof(Camera));
  mat4_set_to_perspective(camera->projection_matrix, fov_y_deg, aspect, near,
                          far);
  return camera;
}

mat4_value_type *camera_get_projection_matrix(const Camera *camera) {
  return (mat4_value_type *)camera->projection_matrix;
}
void camera_destroy(Camera *camera) { memory_free(camera); }
