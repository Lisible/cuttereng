#include "image.h"
#include "asset.h"
#include "memory.h"
#include "png.h"

void image_destroy(Image *image) {
  memory_free(image->data);
  memory_free(image);
}

AssetLoader image_loader = {.fn = image_loader_fn};
void *image_loader_fn(const char *path) {
  char *file_content = asset_read_file_to_string(path);
  Image *image = png_load((u8 *)file_content);
  memory_free(file_content);
  return image;
}

AssetDestructor image_destructor = {.fn = image_destructor_fn};
void image_destructor_fn(void *asset) { image_destroy(asset); }
