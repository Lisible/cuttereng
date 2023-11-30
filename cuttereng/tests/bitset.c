#include "bitset.h"
#include "test.h"

void t_bitset_set() {
  unsigned int bitset[10 / 8 + 1] = {0};
  bitset_set(bitset, 8);
  ASSERT(bitset_is_set(bitset, 8));
}

void t_bitset_unset() {
  unsigned int bitset[10 / 8 + 1] = {0};
  bitset_set(bitset, 8);
  bitset_unset(bitset, 8);
  ASSERT(!bitset_is_set(bitset, 8));
}

TEST_SUITE(TEST(t_bitset_set), TEST(t_bitset_unset))
