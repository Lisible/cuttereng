#include "zlib.h"
#include "src/deflate.h"

bool read_zlib_compressed_data(const u8 *zlib_compressed_data,
                               u8 *out_decompressed_data,
                               const size_t out_decompressed_data_size) {
  LOG_DEBUG("Reading zlib compressed data...");
  u8 cmf = zlib_compressed_data[0];
  u8 cm = cmf & 0x0f;
  LOG_TRACE("cm: %x", cm);
  if (cm != 8) {
    LOG_ERROR("Incompatible compression method: %x\nOnly deflate is "
              "supported (cm=8).",
              cm);
    return false;
  }

  u8 cinfo = cmf >> 4;
  LOG_TRACE("cinfo: %x", cinfo);
  u8 flags = zlib_compressed_data[1];
  bool fdict = ((flags & (1 << 5)) != 0);

  size_t compressed_data_offset = 2;
  if (fdict) {
    compressed_data_offset++;
  }

  // The only currently supported compression method is deflate
  deflate_decompress(zlib_compressed_data + compressed_data_offset,
                     out_decompressed_data, out_decompressed_data_size);

  return true;
}
