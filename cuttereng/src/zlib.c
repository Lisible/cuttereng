#include "zlib.h"
#include "assert.h"
#include "bytes.h"
#include "deflate.h"

ZlibResult read_zlib_compressed_data(const u8 *zlib_compressed_data,
                                     u8 *out_decompressed_data,
                                     const size_t out_decompressed_data_size) {
  ASSERT(zlib_compressed_data != NULL);
  ASSERT(out_decompressed_data != NULL);

  LOG_TRACE("Reading zlib compressed data...");
  u8 cmf = zlib_compressed_data[0];
  u8 cm = cmf & 0x0f;
  LOG_TRACE("cm: %x", cm);
  if (cm != 8) {
    LOG_ERROR("Incompatible compression method: %x\nOnly deflate is "
              "supported (cm=8).",
              cm);
    return ZlibResult_UnsupportedCompressionMethod;
  }

  u8 cinfo = cmf >> 4;
  LOG_TRACE("cinfo: %x", cinfo);
  u8 flags = zlib_compressed_data[1];
  bool fdict = ((flags & (1 << 5)) != 0);
  if (fdict) {
    LOG_ERROR("FDICT flag is unsupported");
    return ZlibResult_UnsupportedFlag;
  }

  static const size_t COMPRESSED_DATA_OFFSET = 2;
  // The only currently supported compression method is deflate
  size_t byte_offset =
      deflate_decompress(zlib_compressed_data + COMPRESSED_DATA_OFFSET,
                         out_decompressed_data, out_decompressed_data_size);

  u32 adler_checksum = u32_from_bytes(zlib_compressed_data +
                                      COMPRESSED_DATA_OFFSET + byte_offset + 1);
  LOG_TRACE("Adler checksum: %d", adler_checksum);

  return ZlibResult_Success;
}
