#include "log.h"
#include "test.h"
#include <filesystem.h>
#include <memory.h>
#include <png.h>

void t_load_simple_png(void) {
  char *file_content = filesystem_read_file_to_string(
      &system_allocator, TEST_DATA_PATH "png/1.png");

  Image *img = png_load(&system_allocator, (u8 *)file_content);
  T_ASSERT(img != NULL);
  for (u32 i = 0; i < img->width * img->height * img->bytes_per_pixel;
       i += img->bytes_per_pixel) {
    int x = (i / img->bytes_per_pixel) % img->width;
    int y = (i / img->bytes_per_pixel) / img->width;

    if (x == 0 && y == 0) {
      T_ASSERT_EQ(img->data[i], 0x00);
      T_ASSERT_EQ(img->data[i + 1], 0x00);
      T_ASSERT_EQ(img->data[i + 2], 0xFF);
    } else if (x == 3 && y == 0) {
      T_ASSERT_EQ(img->data[i], 0x00);
      T_ASSERT_EQ(img->data[i + 1], 0x00);
      T_ASSERT_EQ(img->data[i + 2], 0x00);
    } else if (x == 0 && y == 3) {
      T_ASSERT_EQ(img->data[i], 0xFF);
      T_ASSERT_EQ(img->data[i + 1], 0x00);
      T_ASSERT_EQ(img->data[i + 2], 0x00);
    } else if (x == 3 && y == 3) {
      T_ASSERT_EQ(img->data[i], 0x00);
      T_ASSERT_EQ(img->data[i + 1], 0xFF);
      T_ASSERT_EQ(img->data[i + 2], 0x00);
    } else {
      T_ASSERT_EQ(img->data[i], 0xFF);
      T_ASSERT_EQ(img->data[i + 1], 0xFF);
      T_ASSERT_EQ(img->data[i + 2], 0xFF);
    }
  }

  image_destroy(&system_allocator, img);
  free(file_content);
}

TEST_SUITE(TEST(t_load_simple_png))
