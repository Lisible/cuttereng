#ifndef CUTTERENG_BYTES_H
#define CUTTERENG_BYTES_H

#include "common.h"

u32 u32_from_bytes(const u8 *bytes);
u32 u32_from_bytes_le(const u8 *bytes);
u16 u16_from_bytes_le(const u8 *bytes);
float float_from_bytes_le(const u8 *bytes);

#endif // CUTTERENG_BYTES_H
