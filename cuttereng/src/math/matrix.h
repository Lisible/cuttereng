#ifndef CUTTERENG_MATH_MATRIX_H
#define CUTTERENG_MATH_MATRIX_H

#include "vector.h"
#include <lisiblestd/assert.h>

#define DEFINE_MAT4(T, name)                                                   \
  typedef T name[16];                                                          \
  void name##_transpose(name mat);                                             \
  void name##_mul(name lhs, name rhs, name out);

#define IMPL_MAT4(T, name)                                                     \
  void name##_transpose(name mat) {                                            \
    LSTD_ASSERT(mat != NULL);                                                  \
    for (int row = 0; row < 4; row++) {                                        \
      for (int col = 0; col < row; col++) {                                    \
        T tmp = mat[row * 4 + col];                                            \
        mat[row * 4 + col] = mat[col * 4 + row];                               \
        mat[col * 4 + row] = tmp;                                              \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  void name##_mul(name lhs, name rhs, name out) {                              \
    static const size_t COLS = 4;                                              \
    LSTD_ASSERT(lhs != NULL);                                                  \
    LSTD_ASSERT(rhs != NULL);                                                  \
    for (int j = 0; j < 4; j++) {                                              \
      for (int i = 0; i < 4; i++) {                                            \
        out[j * COLS + i] = lhs[j * COLS] * rhs[i] +                           \
                            lhs[j * COLS + 1] * rhs[i + COLS] +                \
                            lhs[j * COLS + 2] * rhs[i + COLS * 2] +            \
                            lhs[j * COLS + 3] * rhs[i + COLS * 3];             \
      }                                                                        \
    }                                                                          \
  }

// clang-format off
#define MAT4_IDENTITY                                                          \
{ \
  1, 0, 0, 0, \
  0, 1, 0, 0, \
  0, 0, 1, 0, \
  0, 0, 0, 1 \
}
// clang-format on

DEFINE_MAT4(int, mat4i)
DEFINE_MAT4(float, mat4)
DEFINE_MAT4(double, mat4d)

typedef float mat4_value_type;

void mat4_set_to_identity(mat4 mat);
void mat4_set_to_orthographic(mat4 mat, float near, float far, float left,
                              float right, float top, float bottom);
void mat4_set_to_perspective(mat4 mat, float fov_y_deg, float aspect,
                             float near, float far);
void mat4_look_at(mat4 mat, const v3f *eye, const v3f *center, const v3f *up);
void mat4_set_to_translation(mat4 mat, const v3f *translation);
void mat4_set_to_scale(mat4 mat, const v3f *scale);
void mat4_inverse(mat4 mat, mat4 out_mat);

#define MAT4_DEBUG_LOG(mat)                                                    \
  LOG_DEBUG(#mat " : \n(\n %f, %f, %f, %f,\n %f, %f, %f, %f,\n %f, %f, %f, "   \
                 "%f,\n %f, "                                                  \
                 "%f, %f, %f\n)",                                              \
            mat[0], mat[1], mat[2], mat[3], mat[4], mat[5], mat[6], mat[7],    \
            mat[8], mat[9], mat[10], mat[11], mat[12], mat[13], mat[14],       \
            mat[15])

#endif // CUTTERENG_MATH_MATRIX_H
