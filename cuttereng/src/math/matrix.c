#include "matrix.h"
#include "../assert.h"
#include "math.h"
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
void mat4_inverse(mat4 mat, mat4 out_mat) {
  mat4_value_type a2323 =
      mat[2 * 4 + 2] * mat[3 * 4 + 3] - mat[2 * 4 + 3] * mat[3 * 4 + 2];
  mat4_value_type a1323 =
      mat[2 * 4 + 1] * mat[3 * 4 + 3] - mat[2 * 4 + 3] * mat[3 * 4 + 1];
  mat4_value_type a1223 =
      mat[2 * 4 + 1] * mat[3 * 4 + 2] - mat[2 * 4 + 2] * mat[3 * 4 + 1];
  mat4_value_type a0323 =
      mat[2 * 4 + 0] * mat[3 * 4 + 3] - mat[2 * 4 + 3] * mat[3 * 4 + 0];
  mat4_value_type a0223 =
      mat[2 * 4 + 0] * mat[3 * 4 + 2] - mat[2 * 4 + 2] * mat[3 * 4 + 0];
  mat4_value_type a0123 =
      mat[2 * 4 + 0] * mat[3 * 4 + 1] - mat[2 * 4 + 1] * mat[3 * 4 + 0];
  mat4_value_type a2313 =
      mat[1 * 4 + 2] * mat[3 * 4 + 3] - mat[1 * 4 + 3] * mat[3 * 4 + 2];
  mat4_value_type a1313 =
      mat[1 * 4 + 1] * mat[3 * 4 + 3] - mat[1 * 4 + 3] * mat[3 * 4 + 1];
  mat4_value_type a1213 =
      mat[1 * 4 + 1] * mat[3 * 4 + 2] - mat[1 * 4 + 2] * mat[3 * 4 + 1];
  mat4_value_type a2312 =
      mat[1 * 4 + 2] * mat[2 * 4 + 3] - mat[1 * 4 + 3] * mat[2 * 4 + 2];
  mat4_value_type a1312 =
      mat[1 * 4 + 1] * mat[2 * 4 + 3] - mat[1 * 4 + 3] * mat[2 * 4 + 1];
  mat4_value_type a1212 =
      mat[1 * 4 + 1] * mat[2 * 4 + 2] - mat[1 * 4 + 2] * mat[2 * 4 + 1];
  mat4_value_type a0313 =
      mat[1 * 4 + 0] * mat[3 * 4 + 3] - mat[1 * 4 + 3] * mat[3 * 4 + 0];
  mat4_value_type a0213 =
      mat[1 * 4 + 0] * mat[3 * 4 + 2] - mat[1 * 4 + 2] * mat[3 * 4 + 0];
  mat4_value_type a0312 =
      mat[1 * 4 + 0] * mat[2 * 4 + 3] - mat[1 * 4 + 3] * mat[2 * 4 + 0];
  mat4_value_type a0212 =
      mat[1 * 4 + 0] * mat[2 * 4 + 2] - mat[1 * 4 + 2] * mat[2 * 4 + 0];
  mat4_value_type a0113 =
      mat[1 * 4 + 0] * mat[3 * 4 + 1] - mat[1 * 4 + 1] * mat[3 * 4 + 0];
  mat4_value_type a0112 =
      mat[1 * 4 + 0] * mat[2 * 4 + 1] - mat[1 * 4 + 1] * mat[2 * 4 + 0];

  mat4_value_type det =
      mat[0 * 4 + 0] * (mat[1 * 4 + 1] * a2323 - mat[1 * 4 + 2] * a1323 +
                        mat[1 * 4 + 3] * a1223) -
      mat[0 * 4 + 1] * (mat[1 * 4 + 0] * a2323 - mat[1 * 4 + 2] * a0323 +
                        mat[1 * 4 + 3] * a0223) +
      mat[0 * 4 + 2] * (mat[1 * 4 + 0] * a1323 - mat[1 * 4 + 1] * a0323 +
                        mat[1 * 4 + 3] * a0123) -
      mat[0 * 4 + 3] * (mat[1 * 4 + 0] * a1223 - mat[1 * 4 + 1] * a0223 +
                        mat[1 * 4 + 2] * a0123);

  if (fabs(det) < 0.000001) {
    LOG_ERROR("matrix non inversible");
    return;
  }

  mat4_value_type inv_det = 1.0 / det;
  out_mat[0] = inv_det * (mat[1 * 4 + 1] * a2323 - mat[1 * 4 + 2] * a1323 +
                          mat[1 * 4 + 3] * a1223);
  out_mat[1] = inv_det * -(mat[0 * 4 + 1] * a2323 - mat[0 * 4 + 2] * a1323 +
                           mat[0 * 4 + 3] * a1223);
  out_mat[2] = inv_det * (mat[0 * 4 + 1] * a2313 - mat[0 * 4 + 2] * a1313 +
                          mat[0 * 4 + 3] * a1213);
  out_mat[3] = inv_det * -(mat[0 * 4 + 1] * a2312 - mat[0 * 4 + 2] * a1312 +
                           mat[0 * 4 + 3] * a1212);
  out_mat[4] = inv_det * -(mat[1 * 4 + 0] * a2323 - mat[1 * 4 + 2] * a0323 +
                           mat[1 * 4 + 3] * a0223);
  out_mat[5] = inv_det * (mat[0 * 4 + 0] * a2323 - mat[0 * 4 + 2] * a0323 +
                          mat[0 * 4 + 3] * a0223);
  out_mat[6] = inv_det * -(mat[0 * 4 + 0] * a2313 - mat[0 * 4 + 2] * a0313 +
                           mat[0 * 4 + 3] * a0213);
  out_mat[7] = inv_det * (mat[0 * 4 + 0] * a2312 - mat[0 * 4 + 2] * a0312 +
                          mat[0 * 4 + 3] * a0212);
  out_mat[8] = inv_det * (mat[1 * 4 + 0] * a1323 - mat[1 * 4 + 1] * a0323 +
                          mat[1 * 4 + 3] * a0123);
  out_mat[9] = inv_det * -(mat[0 * 4 + 0] * a1323 - mat[0 * 4 + 1] * a0323 +
                           mat[0 * 4 + 3] * a0123);
  out_mat[10] = inv_det * (mat[0 * 4 + 0] * a1313 - mat[0 * 4 + 1] * a0313 +
                           mat[0 * 4 + 3] * a0113);
  out_mat[11] = inv_det * -(mat[0 * 4 + 0] * a1312 - mat[0 * 4 + 1] * a0312 +
                            mat[0 * 4 + 3] * a0112);
  out_mat[12] = inv_det * -(mat[1 * 4 + 0] * a1223 - mat[1 * 4 + 1] * a0223 +
                            mat[1 * 4 + 2] * a0123);
  out_mat[13] = inv_det * (mat[0 * 4 + 0] * a1223 - mat[0 * 4 + 1] * a0223 +
                           mat[0 * 4 + 2] * a0123);
  out_mat[14] = inv_det * -(mat[0 * 4 + 0] * a1213 - mat[0 * 4 + 1] * a0213 +
                            mat[0 * 4 + 2] * a0113);
  out_mat[15] = inv_det * (mat[0 * 4 + 0] * a1212 - mat[0 * 4 + 1] * a0212 +
                           mat[0 * 4 + 2] * a0112);
}
