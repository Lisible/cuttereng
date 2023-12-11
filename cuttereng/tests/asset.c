#include "log.h"
#include "memory.h"
#include "test.h"
#include <asset.h>

struct SomeAssetType;
void *some_asset_type_loader(const char *path) { return NULL; }

struct SomeOtherAssetType;

typedef struct {
  int value;
} IntAsset;
void *int_asset_loader(const char *path) {
  IntAsset *int_asset = memory_allocate(sizeof(IntAsset));
  int_asset->value = 4;
  return int_asset;
}

void int_asset_destructor(void *asset) {
  int *a = asset;
  memory_free(a);
}

void t_assets_register_loader() {
  Assets *assets = assets_new();
  assets_register_loader(assets, SomeAssetType, some_asset_type_loader, NULL);
  ASSERT(assets_is_loader_registered_for_type(assets, SomeAssetType));
  ASSERT(!assets_is_loader_registered_for_type(assets, SomeOtherAssetType));
  assets_destroy(assets);
}

void t_assets_fetch() {
  Assets *assets = assets_new();
  assets_register_loader(assets, IntAsset, int_asset_loader,
                         int_asset_destructor);
  IntAsset *loaded_int_asset =
      assets_fetch(assets, IntAsset, "some/asset_path");

  ASSERT_EQ(loaded_int_asset->value, 4);
  assets_destroy(assets);
}

TEST_SUITE(TEST(t_assets_register_loader), TEST(t_assets_fetch))
