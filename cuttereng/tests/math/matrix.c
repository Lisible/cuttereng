#include "../test.h"
#include "log.h"
#include <math/matrix.h>

void t_mat_mul() {
  mat4i a = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  mat4i b = {3, 2, 1, 6, 5, 4, 9, 8, 7, 12, 11, 10, 15, 14, 13, 16};
  mat4i c = {0};
  mat4i_mul(a, b, c);
  ASSERT_EQ(c[0], 94);
  ASSERT_EQ(c[1], 102);
  ASSERT_EQ(c[2], 104);
  ASSERT_EQ(c[3], 116);
  ASSERT_EQ(c[4], 214);
  ASSERT_EQ(c[5], 230);
  ASSERT_EQ(c[6], 240);
  ASSERT_EQ(c[7], 276);
  ASSERT_EQ(c[8], 334);
  ASSERT_EQ(c[9], 358);
  ASSERT_EQ(c[10], 376);
  ASSERT_EQ(c[11], 436);
  ASSERT_EQ(c[12], 454);
  ASSERT_EQ(c[13], 486);
  ASSERT_EQ(c[14], 512);
  ASSERT_EQ(c[15], 596);
}

void t_mat4_transpose() {
  mat4i a = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  mat4i_transpose(a);
  ASSERT_EQ(a[0], 1);
  ASSERT_EQ(a[1], 5);
  ASSERT_EQ(a[2], 9);
  ASSERT_EQ(a[3], 13);
  ASSERT_EQ(a[4], 2);
  ASSERT_EQ(a[5], 6);
  ASSERT_EQ(a[6], 10);
  ASSERT_EQ(a[7], 14);
  ASSERT_EQ(a[8], 3);
  ASSERT_EQ(a[9], 7);
  ASSERT_EQ(a[10], 11);
  ASSERT_EQ(a[11], 15);
  ASSERT_EQ(a[12], 4);
  ASSERT_EQ(a[13], 8);
  ASSERT_EQ(a[14], 12);
  ASSERT_EQ(a[15], 16);
}

TEST_SUITE(TEST(t_mat_mul), TEST(t_mat4_transpose));
