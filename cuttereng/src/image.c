#include "image.h"
#include "assert.h"
#include "asset.h"
#include "png.h"

void image_destroy(Allocator *allocator, Image *image) {
  ASSERT(image != NULL);
  if (!image) {
    return;
  }

  allocator_free(allocator, image->data);
  allocator_free(allocator, image);
}

AssetLoader image_loader = {.fn = image_loader_fn};
void *image_loader_fn(Allocator *allocator, const char *path) {
  ASSERT(path != NULL);

  char *file_content = asset_read_file_to_string(allocator, path);
  Image *image = png_load(allocator, (u8 *)file_content);
  if (!image) {
    LOG_ERROR("Couldn't load image: %s", path);
  }
  allocator_free(allocator, file_content);
  return image;
}

AssetDestructor image_destructor = {.fn = image_destructor_fn};
void image_destructor_fn(Allocator *allocator, void *asset) {
  image_destroy(allocator, asset);
}
