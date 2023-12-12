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

DEFINE_MAT4(int, mat4i)
DEFINE_MAT4(float, mat4f)
DEFINE_MAT4(double, mat4d)

#endif // CUTTERENG_MATH_MATRIX_H
