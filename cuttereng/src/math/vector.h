#ifndef CUTTERENG_MATH_VECTOR_H
#define CUTTERENG_MATH_VECTOR_H

#define DEFINE_V3(T, name)                                                     \
  typedef struct {                                                             \
    T x;                                                                       \
    T y;                                                                       \
    T z;                                                                       \
  } name;                                                                      \
                                                                               \
  void name##_add(name *lhs, const name *rhs) {                                \
    lhs->x += rhs->x;                                                          \
    lhs->y += rhs->y;                                                          \
    lhs->z += rhs->z;                                                          \
  }                                                                            \
                                                                               \
  void name##_sub(name *lhs, const name *rhs) {                                \
    lhs->x -= rhs->x;                                                          \
    lhs->y -= rhs->y;                                                          \
    lhs->z -= rhs->z;                                                          \
  }                                                                            \
                                                                               \
  void name##_mul_scalar(name *lhs, T rhs) {                                   \
    lhs->x *= rhs;                                                             \
    lhs->y *= rhs;                                                             \
    lhs->z *= rhs;                                                             \
  }                                                                            \
                                                                               \
  void name##_div_scalar(name *lhs, T rhs) {                                   \
    lhs->x /= rhs;                                                             \
    lhs->y /= rhs;                                                             \
    lhs->z /= rhs;                                                             \
  }                                                                            \
                                                                               \
  void name##_neg(name *v) {                                                   \
    v->x = -v->x;                                                              \
    v->y = -v->y;                                                              \
    v->z = -v->z;                                                              \
  }

DEFINE_V3(int, v3i)
DEFINE_V3(float, v3f)
DEFINE_V3(double, v3d)

#endif // CUTTERENG_MATH_VECTOR_H