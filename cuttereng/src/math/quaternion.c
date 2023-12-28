#include "quaternion.h"
#include "../assert.h"

void quaternion_set_to_axis_angle(Quaternion *quaternion, const v3f *axis,
                                  float angle) {
  ASSERT(quaternion != NULL);
  ASSERT(axis != NULL);
  float half_angle = angle / 2.0;
  float half_angle_sin = sin(half_angle);
  float x = axis->x * half_angle_sin;
  float y = axis->y * half_angle_sin;
  float z = axis->z * half_angle_sin;
  float w = cos(half_angle);

  quaternion->scalar_part = w;
  quaternion->vector_part.x = x;
  quaternion->vector_part.y = y;
  quaternion->vector_part.z = z;
}

void quaternion_mul(Quaternion *lhs, const Quaternion *rhs) {
  ASSERT(lhs != NULL);
  ASSERT(rhs != NULL);
  float x1 = lhs->vector_part.x;
  float y1 = lhs->vector_part.y;
  float z1 = lhs->vector_part.z;
  float w1 = lhs->scalar_part;

  float x2 = rhs->vector_part.x;
  float y2 = rhs->vector_part.y;
  float z2 = rhs->vector_part.z;
  float w2 = rhs->scalar_part;

  lhs->scalar_part = w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2;
  lhs->vector_part = (v3f){x1 * w2 + y1 * z2 - z1 * y2 + w1 * x2,
                           y1 * w2 + z1 * x2 + w1 * y2 - x1 * z2,
                           z1 * w2 + w1 * z2 + x1 * y2 - y1 * x2};
}

void quaternion_rotation_matrix(const Quaternion *quaternion,
                                mat4 rotation_matrix) {
  ASSERT(quaternion != NULL);
  ASSERT(rotation_matrix != NULL);
  float w = quaternion->scalar_part;
  float x = quaternion->vector_part.x;
  float y = quaternion->vector_part.y;
  float z = quaternion->vector_part.z;
  float x2 = x + x;
  float y2 = y + y;
  float z2 = z + z;
  float w2 = w + w;
  float xx2 = x2 * x;
  float xy2 = x2 * y;
  float xz2 = x2 * z;
  float yy2 = y2 * y;
  float yz2 = y2 * z;
  float zz2 = z2 * z;
  float wx2 = w2 * x;
  float wy2 = w2 * y;
  float wz2 = w2 * z;

  rotation_matrix[0] = 1.0 - yy2 - zz2;
  rotation_matrix[1] = xy2 - wz2;
  rotation_matrix[2] = xz2 + wy2;
  rotation_matrix[3] = 0.0;
  rotation_matrix[4] = xy2 + wz2;
  rotation_matrix[5] = 1.0 - xx2 - zz2;
  rotation_matrix[6] = yz2 - wx2;
  rotation_matrix[7] = 0.0;
  rotation_matrix[8] = xz2 - wy2;
  rotation_matrix[9] = yz2 + wx2;
  rotation_matrix[10] = 1.0 - xx2 - yy2;
  rotation_matrix[11] = 0.0;
  rotation_matrix[12] = 0.0;
  rotation_matrix[13] = 0.0;
  rotation_matrix[14] = 0.0;
  rotation_matrix[15] = 1.0;
}
