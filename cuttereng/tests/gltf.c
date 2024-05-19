#include "test.h"
#include <environment/environment.h>
#include <gltf.h>
#include <stdio.h>

void t_gltf_parse(void) {
  char *cwd = env_get_cwd_path(&system_allocator);
  size_t cwd_len = strlen(cwd) + 1;
  char *path = "../cuttereng/tests/models/fox.glb";
  size_t path_len = strlen(path);
  size_t actual_path_length = cwd_len + path_len + 1;
  char *actual_path =
      Allocator_allocate_array(&system_allocator, actual_path_length, 1);
  actual_path = strcat(actual_path, cwd);
  actual_path = strcat(actual_path, "/");
  actual_path = strcat(actual_path, path);
  actual_path[actual_path_length - 1] = '\0';

  FILE *f = fopen(actual_path, "r");
  fseek(f, 0, SEEK_END);
  size_t flen = ftell(f);
  fseek(f, 0, SEEK_SET);
  u8 *glb_data = Allocator_allocate(&system_allocator, flen);
  size_t read = fread(glb_data, 1, flen, f);
  fclose(f);
  T_ASSERT(read == flen);

  Gltf *gltf = Gltf_parse_glb(&system_allocator, glb_data, flen);
  T_ASSERT(gltf != NULL);

  Gltf_destroy(&system_allocator, gltf);
  Allocator_free(&system_allocator, glb_data);
  Allocator_free(&system_allocator, cwd);
  Allocator_free(&system_allocator, actual_path);
}

TEST_SUITE(TEST(t_gltf_parse))
