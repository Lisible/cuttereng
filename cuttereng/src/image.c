#include "image.h"
#include "memory.h"

void image_destroy(Image *image) {
  memory_free(image->data);
  memory_free(image);
}
