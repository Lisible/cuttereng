#include "matrix.h"
#include "../assert.h"
#include "radians.h"
#include <string.h>

IMPL_MAT4(int, mat4i)
IMPL_MAT4(float, mat4)
IMPL_MAT4(double, mat4d)

void mat4_set_to_perspective(mat4 mat, float fov_y_deg, float aspect,
                             float near, float far) {
  ASSERT(mat != NULL);
  float fov_y_rad = fov_y_deg * 2.0f * M_PI / 360.0f;

  float focalLength = 1 / tan(fov_y_rad / 2);
  near = 0.01f;
  far = 100.0f;
  float divider = 1 / (focalLength * (far - near));

  mat[0] = 1.0;
  mat[1] = 0.0;
  mat[2] = 0.0;
  mat[3] = 0.0;
  mat[4] = 0.0;
  mat[5] = aspect;
  mat[6] = 0.0;
  mat[7] = 0.0;
  mat[8] = 0.0;
  mat[9] = 0.0;
  mat[10] = far * divider;
  mat[11] = -far * near * divider;
  mat[12] = 0.0;
  mat[13] = 0.0;
  mat[14] = 1.0 / focalLength;
  mat[15] = 0.0;
}
void mat4_set_to_translation(mat4 mat, const v3f *translation) {
  ASSERT(mat != NULL);
  ASSERT(translation != NULL);
  mat[0] = 1.0;
  mat[1] = 0.0;
  mat[2] = 0.0;
  mat[3] = translation->x;
  mat[4] = 0.0;
  mat[5] = 1.0;
  mat[6] = 0.0;
  mat[7] = translation->y;
  mat[8] = 0.0;
  mat[9] = 0.0;
  mat[10] = 1.0;
  mat[11] = translation->z;
  mat[12] = 0.0;
  mat[13] = 0.0;
  mat[14] = 0.0;
  mat[15] = 1.0;
}

void mat4_set_to_scale(mat4 mat, const v3f *scale) {
  ASSERT(mat != NULL);
  ASSERT(scale != NULL);
  mat[0] = scale->x;
  mat[1] = 0.0;
  mat[2] = 0.0;
  mat[3] = 0.0;
  mat[4] = 0.0;
  mat[5] = scale->y;
  mat[6] = 0.0;
  mat[7] = 0.0;
  mat[8] = 0.0;
  mat[9] = 0.0;
  mat[10] = scale->z;
  mat[11] = 0.0;
  mat[12] = 0.0;
  mat[13] = 0.0;
  mat[14] = 0.0;
  mat[15] = 1.0;
}
