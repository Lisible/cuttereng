#include "asset.h"
#include "filesystem.h"
#include "hash.h"
#include "log.h"
#include "memory.h"
#include <string.h>

#define ASSETS_BASE_PATH "assets/"

typedef struct {
  HashTable *assets;
} AssetStore;

AssetStore *asset_store_new(AssetDestructor asset_destructor) {
  AssetStore *asset_store = memory_allocate(sizeof(AssetStore));
  asset_store->assets = hash_table_new(asset_destructor);
  return asset_store;
}

void asset_store_set(AssetStore *asset_store, const char *key, void *asset) {
  hash_table_set(asset_store->assets, key, asset);
}

void *asset_store_get(AssetStore *asset_store, const char *key) {
  return hash_table_get(asset_store->assets, key);
}

void asset_store_remove(AssetStore *asset_store, const char *key) {
  hash_table_remove(asset_store->assets, key);
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
  LOG_DEBUG("Loading asset %s of type %s", asset_path, asset_type);
  AssetLoader loader = hash_table_get(assets->loaders, asset_type);
  if (!loader) {
    LOG_ERROR("No asset loader registered for asset type %s", asset_type);
    return NULL;
  }

  void *asset = loader(asset_path);
  if (!asset) {
    LOG_ERROR("Could not load asset %s of type %s", asset_path, asset_type);
    return NULL;
  }

  AssetStore *asset_store =
      hash_table_AssetStore_get(&assets->asset_stores, asset_type);
  if (!asset_store) {
    LOG_DEBUG("No asset store found for assets of type %s, creating it",
              asset_type);
    hash_table_AssetStore_set(
        &assets->asset_stores, asset_type,
        asset_store_new(hash_table_get(assets->destructors, asset_type)));
    asset_store = hash_table_AssetStore_get(&assets->asset_stores, asset_type);
  }

  asset_store_set(asset_store, asset_path, asset);

  LOG_DEBUG("Asset %s of type %s successfully loaded", asset_path, asset_type);
  return asset;
}

void assets_clear(Assets *assets) {
  LOG_DEBUG("Clearing assets...");
  hash_table_AssetStore_clear(&assets->asset_stores);
}

void *assets_fetch_(Assets *assets, const char *asset_type,
                    const char *asset_path) {
  LOG_DEBUG("Fetching asset %s of type %s", asset_path, asset_type);
  AssetStore *asset_store =
      hash_table_AssetStore_get(&assets->asset_stores, asset_type);

  void *asset = NULL;
  if (asset_store != NULL) {
    asset = asset_store_get(asset_store, asset_path);
  }

  if (!asset) {
    LOG_DEBUG("Asset %s of type %s not found in Assets, trying to load it",
              asset_path, asset_type);
    asset = assets_load_asset(assets, asset_type, asset_path);
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

void assets_remove_(Assets *assets, const char *asset_type,
                    const char *asset_path) {
  LOG_DEBUG("Removing asset %s of type %s", asset_path, asset_type);
  AssetStore *asset_store =
      hash_table_AssetStore_get(&assets->asset_stores, asset_type);
  asset_store_remove(asset_store, asset_path);
}

void assets_destroy(Assets *assets) {
  hash_table_AssetStore_deinit(&assets->asset_stores);
  hash_table_destroy(assets->loaders);
  hash_table_destroy(assets->destructors);
  memory_free(assets);
}
char *asset_read_file_to_string(const char *path) {
  size_t effective_path_length = strlen(ASSETS_BASE_PATH) + strlen(path) + 1;
  char *effective_path = memory_allocate(effective_path_length);
  memset(effective_path, 0, effective_path_length);
  strcat(effective_path, ASSETS_BASE_PATH);
  strcat(effective_path, path);
  return filesystem_read_file_to_string(effective_path);
}
