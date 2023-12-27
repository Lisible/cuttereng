#ifndef CUTTERENG_DEFLATE_H
#define CUTTERENG_DEFLATE_H

#include "common.h"

size_t deflate_decompress(const u8 *compressed_data_set, u8 *decompressed_data,
                          const size_t output_buffer_size);

#endif // CUTTERENG_DEFLATE_H
