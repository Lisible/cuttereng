#ifndef CUTTERENG_DEFLATE_H
#define CUTTERENG_DEFLATE_H

#include "common.h"
#include "vec.h"

typedef enum {
  DeflateStatus_Success = 0,
  DeflateStatus_UnknownSymbol
} DeflateStatus;

DeflateStatus deflate_decompress(const u8 *compressed_data_set,
                                 u8vec *decompressed_data);

#endif // CUTTERENG_DEFLATE_H
