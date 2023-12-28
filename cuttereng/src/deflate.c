#include "deflate.h"
#include "assert.h"
#include "bitstream.h"
#include "memory.h"
#include <string.h>

#define MAX_HUFFMAN_CODE_LENGTH 15
#define MAX_CODES (MAX_LENGTH_CODES + MAX_DISTANCE_CODES)
#define CODE_LENGTH_ALPHABET_MAX_SYMBOL_COUNT 19
#define MAX_LENGTH_CODES 286
#define MAX_DISTANCE_CODES 32

typedef enum {
  DeflateCompressionType_NoCompression = 0x00,
  DeflateCompressionType_FixedHuffmanCodes = 0x01,
  DeflateCompressionType_DynamicHuffmanCodes = 0x02,
  DeflateCompressionType_Reserved = 0x03,
} DeflateCompressionType;

typedef struct {
  u16 *counts;
  u16 *symbols;
} HuffmanTable;

void build_huffman_table_from_codelengths(HuffmanTable *table,
                                          const u16 *codelengths,
                                          u16 codelength_count) {
  ASSERT(table != NULL);
  ASSERT(codelengths != NULL);
  for (int i = 0; i < codelength_count; i++) {
    table->counts[codelengths[i]]++;
  }
  table->counts[0] = 0;

  u16 next_code[MAX_HUFFMAN_CODE_LENGTH + 1] = {0};
  for (int i = 1; i < MAX_HUFFMAN_CODE_LENGTH; i++) {
    next_code[i + 1] = next_code[i] + table->counts[i];
  }

  for (int i = 0; i < codelength_count; i++) {
    if (codelengths[i] == 0) {
      continue;
    }

    table->symbols[next_code[codelengths[i]]++] = i;
  }
}

// assumes the huffman table has been zeroed
void build_codelength_table(HuffmanTable *codelength_table,
                            u16 *codelengths_codelengths, u16 hclen) {
  ASSERT(codelength_table != NULL);
  ASSERT(codelengths_codelengths != NULL);
  static const u8 CODELENGTH_MAPPING[] = {16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
                                          11, 4,  12, 3, 13, 2, 14, 1, 15};
  u16 codelengths[CODE_LENGTH_ALPHABET_MAX_SYMBOL_COUNT] = {0};
  for (int i = 0; i < hclen; i++) {
    codelengths[CODELENGTH_MAPPING[i]] = codelengths_codelengths[i];
  }

  build_huffman_table_from_codelengths(codelength_table, codelengths, hclen);
}

int huffman_table_decode(const HuffmanTable *table, Bitstream *bitstream) {
  ASSERT(table != NULL);
  ASSERT(bitstream != NULL);
  int code = 0;
  int index = 0;
  int first = 0;
  for (int i = 1; i <= MAX_HUFFMAN_CODE_LENGTH; i++) {
    code |= bitstream_next_bits(bitstream, 1);
    int count = table->counts[i];
    if (code - count < first) {
      return table->symbols[index + (code - first)];
    }
    index += count;
    first += count;
    first <<= 1;
    code <<= 1;
  }
  return -1;
}

typedef struct {
  u8vec *output_buffer;
  size_t current_output_buffer_index;
} DeflateOutputState;

// FIXME return result status and add bound checkings on the output buffer
DeflateStatus deflate_decompress_(Bitstream *bitstream,
                                  const HuffmanTable *length_literal_table,
                                  const HuffmanTable *dist_table,
                                  DeflateOutputState *output_state) {
  static const u16 length_size_base[29] = {
      3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
      31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
  static const u16 length_extra_bits[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
                                            1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
                                            4, 4, 4, 4, 5, 5, 5, 5, 0};
  static const u16 distance_offset_base[30] = {
      1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
      33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
      1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
  static const u16 distance_extra_bits[30] = {
      0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
      6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

  ASSERT(bitstream != NULL);
  ASSERT(length_literal_table != NULL);
  ASSERT(dist_table != NULL);
  ASSERT(output_state != NULL);

  int symbol = 0;
  while (symbol != 256) {
    symbol = huffman_table_decode(length_literal_table, bitstream);
    if (symbol < 0) {
      return DeflateStatus_UnknownSymbol;
    } else if (symbol < 256) {
      u8vec_append(output_state->output_buffer, (u8 *)&symbol, 1);
    } else if (symbol > 256) {
      symbol -= 257;
      int length = length_size_base[symbol] +
                   bitstream_next_bits(bitstream, length_extra_bits[symbol]);
      symbol = huffman_table_decode(dist_table, bitstream);
      if (symbol < 0) {
        return DeflateStatus_UnknownSymbol;
      }

      int distance =
          distance_offset_base[symbol] +
          bitstream_next_bits(bitstream, distance_extra_bits[symbol]);
      while (length > 0) {
        size_t output_buffer_length = u8vec_length(output_state->output_buffer);
        u8vec_append(output_state->output_buffer,
                     output_state->output_buffer->data + output_buffer_length -
                         distance,
                     1);
        length--;
      }
    }
  }
  return DeflateStatus_Success;
}

DeflateStatus deflate_decompress(const u8 *compressed_data_set,
                                 u8vec *output_buffer) {
  ASSERT(compressed_data_set != NULL);
  ASSERT(output_buffer != NULL);
  LOG_TRACE("Decompressing deflate data");

  Bitstream bitstream;
  bitstream_init(&bitstream, compressed_data_set, 0);
  DeflateOutputState output_state = {.current_output_buffer_index = 0,
                                     .output_buffer = output_buffer};
  u8 bfinal = 0;
  while (!bfinal) {
    bfinal = bitstream_next_bits(&bitstream, 1);
    u8 btype = bitstream_next_bits(&bitstream, 2);
    u16 hlit = bitstream_next_bits(&bitstream, 5) + 257;
    u8 hdist = bitstream_next_bits(&bitstream, 5) + 1;
    u8 hclen = bitstream_next_bits(&bitstream, 4) + 4;

    if (btype == DeflateCompressionType_NoCompression) {
      UNIMPLEMENTED("No compression deflate is not supported");
    } else {
      if (btype == DeflateCompressionType_FixedHuffmanCodes) {
        UNIMPLEMENTED("Fixed Huffman codes deflate is not supported");
      } else {
        u16 codelengths_codelengths[CODE_LENGTH_ALPHABET_MAX_SYMBOL_COUNT] = {
            0};
        for (int i = 0; i < hclen; i++) {
          codelengths_codelengths[i] = bitstream_next_bits(&bitstream, 3);
        }

        u16 codelength_table_symbols[CODE_LENGTH_ALPHABET_MAX_SYMBOL_COUNT] = {
            0};
        u16 codelength_table_counts[CODE_LENGTH_ALPHABET_MAX_SYMBOL_COUNT] = {
            0};
        HuffmanTable codelength_table = {.symbols = codelength_table_symbols,
                                         .counts = codelength_table_counts};
        build_codelength_table(&codelength_table, codelengths_codelengths, 19);
        u16 lenlit_dist_codelengths[MAX_LENGTH_CODES + MAX_DISTANCE_CODES] = {
            0};

        int index = 0;
        while (index < hlit + hdist) {
          int symbol = huffman_table_decode(&codelength_table, &bitstream);
          if (symbol < 0) {
            return DeflateStatus_UnknownSymbol;
          } else if (symbol < 16) {
            lenlit_dist_codelengths[index++] = symbol;
          } else {
            int repeat = 0;
            int codelength = 0;
            if (symbol == 16) {
              repeat = 3 + bitstream_next_bits(&bitstream, 2);
              codelength = lenlit_dist_codelengths[index - 1];
            } else if (symbol == 17) {
              repeat = 3 + bitstream_next_bits(&bitstream, 3);
            } else {
              repeat = 11 + bitstream_next_bits(&bitstream, 7);
            }

            for (int i = 0; i < repeat; i++) {
              lenlit_dist_codelengths[index++] = codelength;
            }
          }
        }

        // The block must have an end
        ASSERT(lenlit_dist_codelengths[256] > 0);

        u16 length_literal_table_symbols[MAX_LENGTH_CODES] = {0};
        u16 length_literal_table_counts[MAX_LENGTH_CODES] = {0};
        HuffmanTable length_literal_table = {
            .counts = length_literal_table_counts,
            .symbols = length_literal_table_symbols};

        // TODO error handling
        build_huffman_table_from_codelengths(&length_literal_table,
                                             lenlit_dist_codelengths, hlit);

        u16 dist_table_symbols[MAX_DISTANCE_CODES] = {0};
        u16 dist_table_counts[MAX_DISTANCE_CODES] = {0};
        HuffmanTable dist_table = {.counts = dist_table_counts,
                                   .symbols = dist_table_symbols};

        // TODO error handling
        build_huffman_table_from_codelengths(
            &dist_table, lenlit_dist_codelengths + hlit, hdist);

        DeflateStatus status = deflate_decompress_(
            &bitstream, &length_literal_table, &dist_table, &output_state);
        if (status != DeflateStatus_Success) {
          return status;
        }
      }
    }
  }

  return DeflateStatus_Success;
}
