#ifndef CUTTERENG_MATH_VECTOR_H
#define CUTTERENG_MATH_VECTOR_H

#define DEFINE_V3(T, name)                                                     \
  typedef struct {                                                             \
    T x;                                                                       \
    T y;                                                                       \
    T z;                                                                       \
  } name;                                                                      \
                                                                               \
  void name##_add(name *lhs, const name *rhs);                                 \
  void name##_sub(name *lhs, const name *rhs);                                 \
  void name##_mul_scalar(name *lhs, T rhs);                                    \
  void name##_div_scalar(name *lhs, T rhs);                                    \
  void name##_neg(name *v);

#define IMPL_V3(T, name)                                                       \
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
typedef v3f v3;
typedef float v3_value_type;

#define DEFINE_V2(T, name)                                                     \
  typedef struct {                                                             \
    T x;                                                                       \
    T y;                                                                       \
  } name;                                                                      \
                                                                               \
  void name##_add(name *lhs, const name *rhs);                                 \
  void name##_sub(name *lhs, const name *rhs);                                 \
  void name##_mul_scalar(name *lhs, T rhs);                                    \
  void name##_div_scalar(name *lhs, T rhs);                                    \
  void name##_neg(name *v);

#define IMPL_V2(T, name)                                                       \
  void name##_add(name *lhs, const name *rhs) {                                \
    lhs->x += rhs->x;                                                          \
    lhs->y += rhs->y;                                                          \
  }                                                                            \
                                                                               \
  void name##_sub(name *lhs, const name *rhs) {                                \
    lhs->x -= rhs->x;                                                          \
    lhs->y -= rhs->y;                                                          \
  }                                                                            \
                                                                               \
  void name##_mul_scalar(name *lhs, T rhs) {                                   \
    lhs->x *= rhs;                                                             \
    lhs->y *= rhs;                                                             \
  }                                                                            \
                                                                               \
  void name##_div_scalar(name *lhs, T rhs) {                                   \
    lhs->x /= rhs;                                                             \
    lhs->y /= rhs;                                                             \
  }                                                                            \
                                                                               \
  void name##_neg(name *v) {                                                   \
    v->x = -v->x;                                                              \
    v->y = -v->y;                                                              \
  }

DEFINE_V2(int, v2i)
DEFINE_V2(float, v2f)
DEFINE_V2(double, v2d)
typedef v2f v2;
typedef float v2_value_type;

#endif // CUTTERENG_MATH_VECTOR_H
