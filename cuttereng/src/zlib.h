#ifndef CUTTERENG_ZLIB_H
#define CUTTERENG_ZLIB_H

#include "common.h"
#include "log.h"

/// Reads zlib compressed data according to RFC1950
/// https://datatracker.ietf.org/doc/html/rfc1950
bool read_zlib_compressed_data(const u8 *zlib_compressed_data,
                               u8 *out_decompressed_data,
                               const size_t out_decompressed_data_size);

#endif // CUTTERENG_ZLIB_H