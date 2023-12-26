#include "bytes.h"

u32 u32_from_bytes(const u8 *bytes) {
  return (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
}
