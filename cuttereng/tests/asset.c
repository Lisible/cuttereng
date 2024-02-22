#include "test.h"
#include <asset.h>
#include <memory.h>

struct SomeAssetType;
void *some_asset_type_loader_fn(Allocator *allocator, Assets *assets,
                                const char *path) {
  (void)allocator;
  (void)path;
  (void)assets;
  return NULL;
}
static AssetLoader some_asset_type_loader = {.fn = some_asset_type_loader_fn};
void some_asset_type_destructor_fn(Allocator *allocator, void *asset) {
  (void)allocator;
  (void)asset;
}
static AssetDestructor some_asset_type_destructor = {
    .fn = some_asset_type_destructor_fn};

struct SomeOtherAssetType;

typedef struct {
  int value;
} IntAsset;

void *int_asset_loader_fn(Allocator *allocator, Assets *assets,
                          const char *path) {
  (void)path;
  (void)assets;
  IntAsset *int_asset = allocator_allocate(allocator, sizeof(IntAsset));
  int_asset->value = 4;
  return int_asset;
}
static AssetLoader int_asset_loader = {.fn = int_asset_loader_fn};

void int_asset_destructor_fn(Allocator *allocator, void *asset) {
  int *a = asset;
  allocator_free(allocator, a);
}
static AssetDestructor int_asset_destructor = {.fn = int_asset_destructor_fn};

void t_assets_register_loader(void) {
  Assets *assets = assets_new(&system_allocator);
  assets_register_loader(assets, SomeAssetType, &some_asset_type_loader,
                         &some_asset_type_destructor);
  T_ASSERT(assets_is_loader_registered_for_type(assets, SomeAssetType));
  T_ASSERT(!assets_is_loader_registered_for_type(assets, SomeOtherAssetType));
  assets_destroy(assets);
}

void t_assets_fetch(void) {
  Assets *assets = assets_new(&system_allocator);
  assets_register_loader(assets, IntAsset, &int_asset_loader,
                         &int_asset_destructor);
  AssetHandle loaded_int_asset_handle;
  bool load_result = assets_load(assets, IntAsset, "some/asset_path",
                                 &loaded_int_asset_handle);
  if (load_result) {
    IntAsset *loaded_int_asset =
        assets_get(assets, IntAsset, loaded_int_asset_handle);
    T_ASSERT_EQ(loaded_int_asset->value, 4);
  } else {
    T_ASSERT(false);
  }

  assets_destroy(assets);
}

TEST_SUITE(TEST(t_assets_register_loader), TEST(t_assets_fetch))
