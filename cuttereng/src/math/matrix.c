#include "matrix.h"
#include "radians.h"

void mat4_set_to_perspective(mat4 mat, float fov_y_deg, float aspect,
                             float near, float far) {
  float top = near * tan((rad_from_degrees(fov_y_deg) / 2.0));
  float bottom = -top;
  float right = top * aspect;
  float left = -right;

  mat[0] = (2.0 * near) / (right - left);
  mat[1] = 0.0;
  mat[2] = 0.0;
  mat[3] = -near * (right + left) / (right - left);

  mat[4] = 0.0;
  mat[5] = (2 * near) / (top - bottom);
  mat[6] = 0.0;
  mat[7] = -near * (top + bottom) / (top - bottom);

  mat[8] = 0.0;
  mat[9] = 0.0;
  mat[10] = -(far + near) / (far - near);
  mat[11] = 2.0 * far * near / (near - far);

  mat[12] = 0.0;
  mat[13] = 0.0;
  mat[14] = -1.0;
  mat[15] = 0.0;
}
