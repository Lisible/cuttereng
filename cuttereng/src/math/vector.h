#ifndef CUTTERENG_MATH_VECTOR_H
#define CUTTERENG_MATH_VECTOR_H

#define DEFINE_V4(T, name)                                                     \
  typedef struct {                                                             \
    T x;                                                                       \
    T y;                                                                       \
    T z;                                                                       \
    T w;                                                                       \
  } name;                                                                      \
                                                                               \
  void name##_add(name *lhs, const name *rhs);                                 \
  void name##_sub(name *lhs, const name *rhs);                                 \
  void name##_mul_scalar(name *lhs, T rhs);                                    \
  void name##_div_scalar(name *lhs, T rhs);                                    \
  void name##_neg(name *v);                                                    \
  T name##_dot(const name *lhs, const name *rhs);                              \
  T name##_length(const name *v);                                              \
  void name##_normalize(name *v);

#define IMPL_V4(T, name)                                                       \
  void name##_add(name *lhs, const name *rhs) {                                \
    ASSERT(lhs != NULL);                                                       \
    ASSERT(rhs != NULL);                                                       \
    lhs->x += rhs->x;                                                          \
    lhs->y += rhs->y;                                                          \
    lhs->z += rhs->z;                                                          \
    lhs->w += rhs->w;                                                          \
  }                                                                            \
                                                                               \
  void name##_sub(name *lhs, const name *rhs) {                                \
    ASSERT(lhs != NULL);                                                       \
    ASSERT(rhs != NULL);                                                       \
    lhs->x -= rhs->x;                                                          \
    lhs->y -= rhs->y;                                                          \
    lhs->z -= rhs->z;                                                          \
    lhs->w -= rhs->w;                                                          \
  }                                                                            \
                                                                               \
  void name##_mul_scalar(name *lhs, T rhs) {                                   \
    ASSERT(lhs != NULL);                                                       \
    lhs->x *= rhs;                                                             \
    lhs->y *= rhs;                                                             \
    lhs->z *= rhs;                                                             \
    lhs->w *= rhs;                                                             \
  }                                                                            \
                                                                               \
  void name##_div_scalar(name *lhs, T rhs) {                                   \
    ASSERT(lhs != NULL);                                                       \
    lhs->x /= rhs;                                                             \
    lhs->y /= rhs;                                                             \
    lhs->z /= rhs;                                                             \
    lhs->w /= rhs;                                                             \
  }                                                                            \
                                                                               \
  void name##_neg(name *v) {                                                   \
    ASSERT(v != NULL);                                                         \
    v->x = -v->x;                                                              \
    v->y = -v->y;                                                              \
    v->z = -v->z;                                                              \
    v->w = -v->w;                                                              \
  }                                                                            \
  T name##_dot(const name *lhs, const name *rhs) {                             \
    ASSERT(lhs != NULL);                                                       \
    ASSERT(rhs != NULL);                                                       \
    return lhs->x * rhs->x + lhs->y * rhs->y + lhs->z * rhs->z +               \
           lhs->w * rhs->w;                                                    \
  }                                                                            \
  T name##_length(const name *v) {                                             \
    return sqrt(v->x * v->x + v->y * v->y + v->z * v->z + v->w * v->w);        \
  }                                                                            \
  void name##_normalize(name *v) {                                             \
    T length = name##_length(v);                                               \
    v->x /= length;                                                            \
    v->y /= length;                                                            \
    v->z /= length;                                                            \
    v->w /= length;                                                            \
  }

DEFINE_V4(float, v4f)

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
  void name##_neg(name *v);                                                    \
  T name##_dot(const name *lhs, const name *rhs);                              \
  void name##_cross(name *lhs, const name *rhs);                               \
  T name##_length(const name *v);                                              \
  void name##_normalize(name *v);

#define IMPL_V3(T, name)                                                       \
  void name##_add(name *lhs, const name *rhs) {                                \
    ASSERT(lhs != NULL);                                                       \
    ASSERT(rhs != NULL);                                                       \
    lhs->x += rhs->x;                                                          \
    lhs->y += rhs->y;                                                          \
    lhs->z += rhs->z;                                                          \
  }                                                                            \
                                                                               \
  void name##_sub(name *lhs, const name *rhs) {                                \
    ASSERT(lhs != NULL);                                                       \
    ASSERT(rhs != NULL);                                                       \
    lhs->x -= rhs->x;                                                          \
    lhs->y -= rhs->y;                                                          \
    lhs->z -= rhs->z;                                                          \
  }                                                                            \
                                                                               \
  void name##_mul_scalar(name *lhs, T rhs) {                                   \
    ASSERT(lhs != NULL);                                                       \
    lhs->x *= rhs;                                                             \
    lhs->y *= rhs;                                                             \
    lhs->z *= rhs;                                                             \
  }                                                                            \
                                                                               \
  void name##_div_scalar(name *lhs, T rhs) {                                   \
    ASSERT(lhs != NULL);                                                       \
    lhs->x /= rhs;                                                             \
    lhs->y /= rhs;                                                             \
    lhs->z /= rhs;                                                             \
  }                                                                            \
                                                                               \
  void name##_neg(name *v) {                                                   \
    ASSERT(v != NULL);                                                         \
    v->x = -v->x;                                                              \
    v->y = -v->y;                                                              \
    v->z = -v->z;                                                              \
  }                                                                            \
  T name##_dot(const name *lhs, const name *rhs) {                             \
    ASSERT(lhs != NULL);                                                       \
    ASSERT(rhs != NULL);                                                       \
    return lhs->x * rhs->x + lhs->y * rhs->y + lhs->z * rhs->z;                \
  }                                                                            \
  void name##_cross(name *lhs, const name *rhs) {                              \
    T lx = lhs->x;                                                             \
    T ly = lhs->y;                                                             \
    T lz = lhs->z;                                                             \
    lhs->x = ly * rhs->z - lz * rhs->y;                                        \
    lhs->y = lz * rhs->x - lx * rhs->z;                                        \
    lhs->z = lx * rhs->y - ly * rhs->x;                                        \
  }                                                                            \
  T name##_length(const name *v) {                                             \
    return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);                      \
  }                                                                            \
  void name##_normalize(name *v) {                                             \
    T length = name##_length(v);                                               \
    v->x /= length;                                                            \
    v->y /= length;                                                            \
    v->z /= length;                                                            \
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
  T name##_length(const name *v);                                              \
  void name##_neg(name *v);

#define IMPL_V2(T, name)                                                       \
  void name##_add(name *lhs, const name *rhs) {                                \
    ASSERT(lhs != NULL);                                                       \
    ASSERT(rhs != NULL);                                                       \
    lhs->x += rhs->x;                                                          \
    lhs->y += rhs->y;                                                          \
  }                                                                            \
                                                                               \
  void name##_sub(name *lhs, const name *rhs) {                                \
    ASSERT(lhs != NULL);                                                       \
    ASSERT(rhs != NULL);                                                       \
    lhs->x -= rhs->x;                                                          \
    lhs->y -= rhs->y;                                                          \
  }                                                                            \
                                                                               \
  void name##_mul_scalar(name *lhs, T rhs) {                                   \
    ASSERT(lhs != NULL);                                                       \
    lhs->x *= rhs;                                                             \
    lhs->y *= rhs;                                                             \
  }                                                                            \
                                                                               \
  void name##_div_scalar(name *lhs, T rhs) {                                   \
    ASSERT(lhs != NULL);                                                       \
    lhs->x /= rhs;                                                             \
    lhs->y /= rhs;                                                             \
  }                                                                            \
  T name##_length(const name *v) { return sqrt(v->x * v->x + v->y * v->y); }   \
                                                                               \
  void name##_neg(name *v) {                                                   \
    ASSERT(v != NULL);                                                         \
    v->x = -v->x;                                                              \
    v->y = -v->y;                                                              \
  }

DEFINE_V2(int, v2i)
DEFINE_V2(float, v2f)
DEFINE_V2(double, v2d)
typedef v2f v2;
typedef float v2_value_type;

#endif // CUTTERENG_MATH_VECTOR_H
