#include "image.h"
#include "asset.h"
#include "lisiblepng.h"
#include <lisiblestd/assert.h>
#include <string.h>

PixelFormat PixelFormat_from_LisPngColourType(LisPngColourType colour_type) {
  switch (colour_type) {
  case LisPngColourType_Truecolour:
    return PixelFormat_R8G8B8;
  default:
    return PixelFormat_Unknown;
  }
}

void Image_destroy(Allocator *allocator, Image *image) {
  LSTD_ASSERT(image != NULL);
  if (!image) {
    return;
  }

  Allocator_free(allocator, image);
}

Image *Image_from_png(Allocator *allocator, LisPng *png) {
  Image *image = Allocator_allocate(allocator, sizeof(Image));
  if (!image) {
    return NULL;
  }

  LisPngColourType colour_type = LisPng_colour_type(png);
  image->pixel_format = PixelFormat_from_LisPngColourType(colour_type);
  image->bytes_per_pixel = (LisPng_bits_per_sample(png) *
                            LisPngColourType_sample_count(colour_type)) /
                           8;

  image->width = LisPng_width(png);
  image->height = LisPng_height(png);
  image->data =
      Allocator_allocate(allocator, image->width * image->height * 32);
  LisPng_write_RGBA8_data(png, image->data);
  image->pixel_format = PixelFormat_R8G8B8A8;
  image->bytes_per_pixel = 4;
  return image;
}

AssetLoader image_loader = {.fn = Image_loader_fn};
void *Image_loader_fn(Allocator *allocator, Assets *assets, const char *path) {
  LSTD_ASSERT(path != NULL);
  (void)assets;

  size_t png_file_content_size;
  char *png_file_content =
      asset_read_file_to_string(allocator, path, &png_file_content_size);
  FILE *png_data_stream =
      fmemopen(png_file_content, png_file_content_size, "r");
  LisPng *png = LisPng_decode(png_data_stream);
  fclose(png_data_stream);
  Image *image = Image_from_png(allocator, png);
  Allocator_free(allocator, png_file_content);
  LisPng_destroy(png);
  return image;
}

AssetDeinitializer image_deinitializer = {.fn = Image_deinitializer_fn};
void Image_deinitializer_fn(Allocator *allocator, void *asset) {
  Image *image = asset;
  Allocator_free(allocator, image->data);
}
