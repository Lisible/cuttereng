#include "test.h"
#include <bitstream.h>
#include <log.h>

void t_bitstream_init(void) {
  const u8 bytes[] = {0b00100011, 0b11110000, 0b10101101};
  Bitstream bitstream;
  bitstream_init(&bitstream, bytes, 3);
  T_ASSERT_EQ(bitstream.current_byte_index, 0);
  T_ASSERT_EQ(bitstream.current_bit_offset, 0);
  T_ASSERT_NOT_NULL(bitstream.data);
}

void t_bitstream_read_first_bits(void) {
  const u8 bytes[] = {0b00100011, 0b11110000, 0b10101101};
  Bitstream bitstream;
  bitstream_init(&bitstream, bytes, 3);
  u8 three_first_bits = bitstream_next_bits(&bitstream, 3);
  T_ASSERT_EQ(three_first_bits, 0b011);
  u8 next_three_bits = bitstream_next_bits(&bitstream, 3);
  T_ASSERT_EQ(next_three_bits, 0b100);
}

void t_bitstream_read_bits_spanning_over_two_bytes(void) {
  const u8 bytes[] = {0b00100011, 0b11110101, 0b10101101};
  Bitstream bitstream;
  bitstream_init(&bitstream, bytes, 3);
  bitstream_skip(&bitstream, 3);
  u8 next_8_bits = bitstream_next_bits(&bitstream, 8);
  T_ASSERT_EQ(next_8_bits, 0b10100100);
  u8 next_7_bits = bitstream_next_bits(&bitstream, 7);
  T_ASSERT_EQ(next_7_bits, 0b0111110);
}

TEST_SUITE(TEST(t_bitstream_init), TEST(t_bitstream_read_first_bits),
           TEST(t_bitstream_read_bits_spanning_over_two_bytes))
