#include "../test.h"
#include <lisiblestd/log.h>
#include <math/matrix.h>

void t_mat_mul(void) {
  mat4i a = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  mat4i b = {3, 2, 1, 6, 5, 4, 9, 8, 7, 12, 11, 10, 15, 14, 13, 16};
  mat4i c = {0};
  mat4i_mul(a, b, c);
  T_ASSERT_EQ(c[0], 94);
  T_ASSERT_EQ(c[1], 102);
  T_ASSERT_EQ(c[2], 104);
  T_ASSERT_EQ(c[3], 116);
  T_ASSERT_EQ(c[4], 214);
  T_ASSERT_EQ(c[5], 230);
  T_ASSERT_EQ(c[6], 240);
  T_ASSERT_EQ(c[7], 276);
  T_ASSERT_EQ(c[8], 334);
  T_ASSERT_EQ(c[9], 358);
  T_ASSERT_EQ(c[10], 376);
  T_ASSERT_EQ(c[11], 436);
  T_ASSERT_EQ(c[12], 454);
  T_ASSERT_EQ(c[13], 486);
  T_ASSERT_EQ(c[14], 512);
  T_ASSERT_EQ(c[15], 596);
}

void t_mat4_transpose(void) {
  mat4i a = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  mat4i_transpose(a);
  T_ASSERT_EQ(a[0], 1);
  T_ASSERT_EQ(a[1], 5);
  T_ASSERT_EQ(a[2], 9);
  T_ASSERT_EQ(a[3], 13);
  T_ASSERT_EQ(a[4], 2);
  T_ASSERT_EQ(a[5], 6);
  T_ASSERT_EQ(a[6], 10);
  T_ASSERT_EQ(a[7], 14);
  T_ASSERT_EQ(a[8], 3);
  T_ASSERT_EQ(a[9], 7);
  T_ASSERT_EQ(a[10], 11);
  T_ASSERT_EQ(a[11], 15);
  T_ASSERT_EQ(a[12], 4);
  T_ASSERT_EQ(a[13], 8);
  T_ASSERT_EQ(a[14], 12);
  T_ASSERT_EQ(a[15], 16);
}

void t_mat4_inverse(void) {
  mat4 a = {
      1.0, 0.0, 0.0, 1.0, 0.0, 2.0, 1.0, 2.0,
      2.0, 1.0, 0.0, 1.0, 2.0, 0.0, 1.0, 4.0,
  };
  mat4 a_inv;
  mat4_inverse(a, a_inv);
  T_ASSERT_FLOAT_EQ(a_inv[0], -2.0, 0.01);
  T_ASSERT_FLOAT_EQ(a_inv[1], -0.5, 0.01);
  T_ASSERT_FLOAT_EQ(a_inv[2], 1.0, 0.01);
  T_ASSERT_FLOAT_EQ(a_inv[3], 0.5, 0.01);
  T_ASSERT_FLOAT_EQ(a_inv[4], 1.0, 0.01);
  T_ASSERT_FLOAT_EQ(a_inv[5], 0.5, 0.01);
  T_ASSERT_FLOAT_EQ(a_inv[6], 0.0, 0.01);
  T_ASSERT_FLOAT_EQ(a_inv[7], -0.5, 0.01);
  T_ASSERT_FLOAT_EQ(a_inv[8], -8.0, 0.01);
  T_ASSERT_FLOAT_EQ(a_inv[9], -1.0, 0.01);
  T_ASSERT_FLOAT_EQ(a_inv[10], 2.0, 0.01);
  T_ASSERT_FLOAT_EQ(a_inv[11], 2.0, 0.01);
  T_ASSERT_FLOAT_EQ(a_inv[12], 3.0, 0.01);
  T_ASSERT_FLOAT_EQ(a_inv[13], 0.5, 0.01);
  T_ASSERT_FLOAT_EQ(a_inv[14], -1.0, 0.01);
  T_ASSERT_FLOAT_EQ(a_inv[15], -0.5, 0.01);
}

TEST_SUITE(TEST(t_mat_mul), TEST(t_mat4_transpose), TEST(t_mat4_inverse))
