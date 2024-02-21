#include "bytes.h"
#include "assert.h"
#include <string.h>

u32 u32_from_bytes(const u8 *bytes) {
  ASSERT(bytes != NULL);
  return (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
}
u32 u32_from_bytes_le(const u8 *bytes) {
  ASSERT(bytes != NULL);
  return (bytes[3] << 24) + (bytes[2] << 16) + (bytes[1] << 8) + bytes[0];
}

u16 u16_from_bytes_le(const u8 *bytes) {
  ASSERT(bytes != NULL);
  return (bytes[1] << 8) + bytes[0];
}

float float_from_bytes_le(const u8 *bytes) {
  ASSERT(bytes != NULL);
  float result;
  memcpy(&result, bytes, sizeof(float));
  return result;
}
