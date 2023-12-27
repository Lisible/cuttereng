#ifndef CUTTERENG_PNG_H
#define CUTTERENG_PNG_H

#include "common.h"
#include "image.h"

Image *png_load(const u8 *datastream);

#define LOG_PNG_PREFIX "PNG decoder: "
#define LOG_PNG_DECODER(msg, ...) LOG_TRACE(LOG_PNG_PREFIX msg, ##__VA_ARGS__)
#define LOG_PNG_DECODER_ERROR(msg, ...)                                        \
  LOG_ERROR(LOG_PNG_PREFIX msg, ##__VA_ARGS__)

#endif // CUTTERENG_PNG_H
