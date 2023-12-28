#ifndef CUTTERENG_IMAGE_H
#define CUTTERENG_IMAGE_H

#include "asset.h"
#include "common.h"

typedef enum {
  PixelFormat_R8G8B8,
  PixelFormat_R8G8B8A8,
  PixelFormat_Unknown,
} PixelFormat;

typedef struct {
  u32 width;
  u32 height;
  u32 bytes_per_pixel;
  PixelFormat pixel_format;
  u8 *data;
} Image;

void image_destroy(Image *image);

void *image_loader_fn(const char *path);
extern AssetLoader image_loader;

void image_destructor_fn(void *asset);
extern AssetDestructor image_destructor;

#endif // CUTTERENG_IMAGE_H
