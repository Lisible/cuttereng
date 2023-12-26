#ifndef CUTTERENG_BITSTREAM_H
#define CUTTERENG_BITSTREAM_H

#include "common.h"

typedef struct {
  const u8 *data;
  size_t data_size;

  size_t current_byte_index;
  u8 current_bit_offset;
} Bitstream;

void bitstream_init(Bitstream *bitstream, const u8 *data, size_t data_size);
void bitstream_skip(Bitstream *bitstream, size_t bit_count);
u16 bitstream_next_bits(Bitstream *bitstream, int bit_count);

#endif // CUTTERENG_BITSTREAM_H
