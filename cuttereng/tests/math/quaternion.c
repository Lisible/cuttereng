#include "../test.h"
#include <common.h>
#include <math/quaternion.h>
#include <math/vector.h>

void t_quaternion_apply_to_vector(void) {
  Quaternion rotation;
  quaternion_set_to_axis_angle(&rotation, &(const v3f){.y = 1.0}, M_PI);
  v3f rotated_vector = {1.0, 0.0, 0.0};
  quaternion_apply_to_vector(&rotation, &rotated_vector);
  T_ASSERT_EQ((i32)rotated_vector.x, -1);
  T_ASSERT_EQ((i32)rotated_vector.y, 0);
  T_ASSERT_EQ((i32)rotated_vector.z, 0);
}

TEST_SUITE(TEST(t_quaternion_apply_to_vector))
