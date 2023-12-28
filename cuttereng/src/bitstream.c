#include "bitstream.h"
#include "assert.h"

void bitstream_init(Bitstream *bitstream, const u8 *data, size_t data_size) {
  ASSERT(bitstream != NULL);
  ASSERT(data != NULL);
  bitstream->data = data;
  bitstream->data_size = data_size;
  bitstream->current_byte_index = 0;
  bitstream->current_bit_offset = 0;
}

void bitstream_advance(Bitstream *bitstream, size_t bit_count) {
  ASSERT(bitstream != NULL);
  u8 current_bit = bitstream->current_bit_offset;
  bitstream->current_byte_index =
      bitstream->current_byte_index + (current_bit + bit_count) / 8;
  bitstream->current_bit_offset = (current_bit + bit_count) % 8;
}

void bitstream_skip(Bitstream *bitstream, size_t bit_count) {
  ASSERT(bitstream != NULL);
  bitstream_advance(bitstream, bit_count);
}

u16 bitstream_next_bits(Bitstream *bitstream, int bit_count) {
  ASSERT(bitstream != NULL);
  int bit_to_read = bit_count;
  u16 val = bitstream->data[bitstream->current_byte_index] >>
            bitstream->current_bit_offset;

  size_t bit_read = MIN(bit_count, 8 - bitstream->current_bit_offset);
  bitstream_advance(bitstream, bit_read);
  bit_to_read -= bit_read;
  while (bit_to_read > 0) {
    val |= (bitstream->data[bitstream->current_byte_index] >>
            bitstream->current_bit_offset)
           << bit_read;
    bit_read = MIN(bit_to_read, 8 - bitstream->current_bit_offset);
    bitstream_advance(bitstream, bit_read);
    bit_to_read -= bit_read;
  }

  return (val & ((1 << bit_count) - 1));
}
