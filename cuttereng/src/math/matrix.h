#ifndef CUTTERENG_MATH_MATRIX_H
#define CUTTERENG_MATH_MATRIX_H
#include "../common.h"

#define DEFINE_MAT4(T, name)                                                   \
  typedef T name[16];                                                          \
  void name##_mul(name lhs, name rhs, name out) {                              \
    static const size_t COLS = 4;                                              \
    for (int j = 0; j < 4; j++) {                                              \
      for (int i = 0; i < 4; i++) {                                            \
        out[j * COLS + i] = lhs[j * COLS] * rhs[i] +                           \
                            lhs[j * COLS + 1] * rhs[i + COLS] +                \
                            lhs[j * COLS + 2] * rhs[i + COLS * 2] +            \
                            lhs[j * COLS + 3] * rhs[i + COLS * 3];             \
      }                                                                        \
    }                                                                          \
  }

DEFINE_MAT4(int, mat4i);
DEFINE_MAT4(float, mat4f);
DEFINE_MAT4(double, mat4d);

typedef mat4f mat4;
typedef float mat4_value_type;

// clang-format off
static const mat4 OPENGL_TO_WGPU_MATRIX = {
  1.0, 0.0, 0.0, 0.0,
  0.0, 1.0, 0.0, 0.0,
  0.0, 0.0, 0.5, 0.0,
  0.0, 0.0, 0.5, 1.0
};
// clang-format on

void mat4_set_to_perspective(mat4 mat, float fov_y_deg, float aspect,
                             float near, float far);

#endif // CUTTERENG_MATH_MATRIX_H
