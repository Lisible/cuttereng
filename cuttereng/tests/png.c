#include "log.h"
#include "test.h"
#include <filesystem.h>
#include <png.h>

void load_simple_png() {
  const char *file_content =
      filesystem_read_file_to_string(TEST_DATA_PATH "png/1.png");

  Image *img = png_load((u8 *)file_content);
  ASSERT(img != NULL);
}

TEST_SUITE(TEST(load_simple_png))
