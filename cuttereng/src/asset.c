#include "asset.h"
#include "hash.h"
#include "memory.h"

typedef struct {
  HashTable *assets;
} AssetStore;

AssetStore *asset_store_new(AssetDestructor asset_destructor) {
  AssetStore *asset_store = memory_allocate(sizeof(AssetStore));
  asset_store->assets = hash_table_new(asset_destructor);
  return asset_store;
}

void asset_store_destroy(AssetStore *asset_store) {
  hash_table_destroy(asset_store->assets);
  memory_free(asset_store);
}

DefineHashTableOf(AssetStore, asset_store_destroy);
struct Assets {
  HashTable *loaders;
  HashTable *destructors;
  HashTableOf(AssetStore) asset_stores;
};

Assets *assets_new() {
  Assets *assets = memory_allocate(sizeof(Assets));
  assets->loaders = hash_table_new(NULL);
  assets->destructors = hash_table_new(NULL);
  hash_table_AssetStore_init(&assets->asset_stores);
  return assets;
}

void *assets_load_asset(Assets *assets, const char *asset_type,
                        const char *asset_path) {
  AssetLoader loader = hash_table_get(assets->loaders, asset_type);
  if (!loader) {
    LOG_ERROR("No asset loader registered for asset type %s\n", asset_type);
    return NULL;
  }

  void *asset = loader(asset_path);
  AssetStore *asset_store =
      hash_table_AssetStore_get(&assets->asset_stores, asset_type);
  if (!asset_store) {
    hash_table_AssetStore_set(
        &assets->asset_stores, asset_type,
        asset_store_new(hash_table_get(assets->destructors, asset_type)));
    asset_store = hash_table_AssetStore_get(&assets->asset_stores, asset_type);
  }

  hash_table_set(asset_store->assets, asset_path, asset);
  return asset;
}

void *assets_fetch_(Assets *assets, const char *asset_type,
                    const char *asset_name) {
  AssetStore *asset_store =
      hash_table_AssetStore_get(&assets->asset_stores, asset_type);

  void *asset = NULL;
  if (asset_store != NULL) {
    asset = hash_table_get(asset_store->assets, asset_name);
  }
  if (!asset) {
    asset = assets_load_asset(assets, asset_type, asset_name);
  }

  return asset;
}

void assets_register_loader_(Assets *assets, const char *asset_type,
                             AssetLoader asset_loader,
                             AssetDestructor asset_destructor) {
  hash_table_set(assets->loaders, asset_type, asset_loader);
  hash_table_set(assets->destructors, asset_type, asset_destructor);
}
bool assets_is_loader_registered_for_type_(const Assets *assets,
                                           const char *asset_type) {
  return hash_table_has(assets->loaders, asset_type);
}

void assets_destroy(Assets *assets) {
  hash_table_AssetStore_deinit(&assets->asset_stores);
  hash_table_destroy(assets->loaders);
  hash_table_destroy(assets->destructors);
  memory_free(assets);
}
