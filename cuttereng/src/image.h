#ifndef CUTTERENG_IMAGE_H
#define CUTTERENG_IMAGE_H

#include "asset.h"
#include "common.h"
#include "memory.h"
#include <lisiblepng.h>

typedef enum {
  PixelFormat_R8G8B8 = 0,
  PixelFormat_R8G8B8A8,
  PixelFormat_Unknown,
} PixelFormat;
PixelFormat PixelFormat_from_LisPngColourType(LisPngColourType colour_type);

typedef struct {
  u8 *data;
  u32 width;
  u32 height;
  u32 bytes_per_pixel;
  PixelFormat pixel_format;
} Image;

Image *Image_from_png(Allocator *allocator, LisPng *png);
void Image_destroy(Allocator *allocator, Image *image);

void *Image_loader_fn(Allocator *allocator, Assets *assets, const char *path);
extern AssetLoader image_loader;

void Image_deinitializer_fn(Allocator *allocator, void *asset);
extern AssetDeinitializer image_deinitializer;

#endif // CUTTERENG_IMAGE_H
