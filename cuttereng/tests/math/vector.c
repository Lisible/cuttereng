#include "../test.h"
#include "log.h"
#include <math/vector.h>

void t_v_add(void) {
  v3i a = {1, 2, 3};
  v3i b = {4, 5, 6};
  v3i_add(&a, &b);

  ASSERT_EQ(a.x, 5);
  ASSERT_EQ(a.y, 7);
  ASSERT_EQ(a.z, 9);
}

void t_v_sub(void) {
  v3i a = {1, 2, 3};
  v3i b = {4, 5, 6};
  v3i_sub(&a, &b);

  ASSERT_EQ(a.x, -3);
  ASSERT_EQ(a.y, -3);
  ASSERT_EQ(a.z, -3);
}

void t_v_neg(void) {
  v3i a = {1, 2, 3};
  v3i_neg(&a);

  ASSERT_EQ(a.x, -1);
  ASSERT_EQ(a.y, -2);
  ASSERT_EQ(a.z, -3);
}

void t_v_mul_scalar(void) {
  v3i a = {1, 2, 3};
  int b = 2;
  v3i_mul_scalar(&a, b);

  ASSERT_EQ(a.x, 2);
  ASSERT_EQ(a.y, 4);
  ASSERT_EQ(a.z, 6);
}

void t_v_div_scalar(void) {
  v3i a = {2, 4, 6};
  int b = 2;
  v3i_div_scalar(&a, b);

  ASSERT_EQ(a.x, 1);
  ASSERT_EQ(a.y, 2);
  ASSERT_EQ(a.z, 3);
}

TEST_SUITE(TEST(t_v_add), TEST(t_v_sub), TEST(t_v_neg), TEST(t_v_mul_scalar),
           TEST(t_v_div_scalar))
