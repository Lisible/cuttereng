#include "image.h"
#include "assert.h"
#include "asset.h"
#include "lisiblepng.h"
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
  ASSERT(image != NULL);
  if (!image) {
    return;
  }

  allocator_free(allocator, image);
}

Image *Image_from_png(Allocator *allocator, LisPng *png) {
  Image *image = allocator_allocate(allocator, sizeof(Image));
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
      allocator_allocate(allocator, image->width * image->height * 32);
  LisPng_write_RGBA8_data(png, image->data);
  image->pixel_format = PixelFormat_R8G8B8A8;
  image->bytes_per_pixel = 4;
  return image;
}

AssetLoader image_loader = {.fn = Image_loader_fn};
void *Image_loader_fn(Allocator *allocator, Assets *assets, const char *path) {
  ASSERT(path != NULL);
  (void)assets;

  size_t png_file_content_size;
  char *png_file_content =
      asset_read_file_to_string(allocator, path, &png_file_content_size);
  FILE *png_data_stream =
      fmemopen(png_file_content, png_file_content_size, "r");
  LisPng *png = LisPng_decode(png_data_stream);
  fclose(png_data_stream);
  Image *image = Image_from_png(allocator, png);
  allocator_free(allocator, png_file_content);
  LisPng_destroy(png);
  return image;
}

AssetDestructor image_destructor = {.fn = Image_destructor_fn};
void Image_destructor_fn(Allocator *allocator, void *asset) {
  Image *image = asset;
  allocator_free(allocator, image->data);
}
