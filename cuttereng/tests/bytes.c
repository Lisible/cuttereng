#include "test.h"
#include <bytes.h>
#include <common.h>

void t_float_from_bytes_le(void) {
  const u8 bytes[4] = {154, 153, 137, 64};
  float number = float_from_bytes_le(bytes);
  T_ASSERT_FLOAT_EQ(number, 4.3, 0.01);
}

TEST_SUITE(TEST(t_float_from_bytes_le))
