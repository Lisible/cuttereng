#ifndef CUTTERENG_ZLIB_H
#define CUTTERENG_ZLIB_H

#include "common.h"
#include "log.h"
#include "vec.h"

typedef enum {
  ZlibResult_Success,
  ZlibResult_DeflateDecompressionFailed,
  ZlibResult_UnsupportedCompressionMethod,
  ZlibResult_UnsupportedFlag,
} ZlibResult;

/// Reads zlib compressed data according to RFC1950
/// https://datatracker.ietf.org/doc/html/rfc1950
ZlibResult read_zlib_compressed_data(const u8 *zlib_compressed_data,
                                     u8vec *out_decompressed_data);

#endif // CUTTERENG_ZLIB_H
