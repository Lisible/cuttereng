#ifndef CUTTERENG_MATH_QUATERNION_H
#define CUTTERENG_MATH_QUATERNION_H
#include "matrix.h"
#include "vector.h"
#include <math.h>

typedef struct {
  float scalar_part;
  v3f vector_part;
} Quaternion;

void quaternion_set_to_axis_angle(Quaternion *quaternion, const v3f *axis,
                                  float angle);
void quaternion_mul(Quaternion *lhs, const Quaternion *rhs);
void quaternion_rotation_matrix(const Quaternion *quaternion,
                                mat4 rotation_matrix);

#endif // CUTTERENG_MATH_QUATERNION_H
