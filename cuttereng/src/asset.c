#include "asset.h"
#include "assert.h"
#include "filesystem.h"
#include "hash.h"
#include "log.h"
#include "memory.h"
#include <string.h>

#define ASSETS_BASE_PATH "assets/"

DECL_HASH_TABLE(void *, HashTableAsset)
DEF_HASH_TABLE(void *, HashTableAsset, HashTable_noop_destructor)

typedef struct {
  HashTableAsset *assets;
} AssetStore;

AssetStore *asset_store_new(AssetDestructorFn asset_destructor) {
  AssetStore *asset_store = memory_allocate(sizeof(AssetStore));
  asset_store->assets = HashTableAsset_create(16);
  return asset_store;
}

void asset_store_set(AssetStore *asset_store, char *key, void *asset) {
  HashTableAsset_set(asset_store->assets, key, asset);
}

void *asset_store_get(AssetStore *asset_store, const char *key) {
  return HashTableAsset_get(asset_store->assets, key);
}

void asset_store_remove(AssetStore *asset_store, const char *key) {
  HashTableAsset_remove(asset_store->assets, key);
}

void asset_store_destroy(AssetStore *asset_store) {
  HashTableAsset_destroy(asset_store->assets);
  memory_free(asset_store);
}

DECL_HASH_TABLE(AssetLoader, HashTableAssetLoader);
DEF_HASH_TABLE(AssetLoader, HashTableAssetLoader, HashTable_noop_destructor);
DECL_HASH_TABLE(AssetDestructor, HashTableAssetDestructor);
DEF_HASH_TABLE(AssetDestructor, HashTableAssetDestructor,
               HashTable_noop_destructor);
DECL_HASH_TABLE(AssetStore, HashTableAssetStore);
DEF_HASH_TABLE(AssetStore, HashTableAssetStore, asset_store_destroy);

struct Assets {
  HashTableAssetLoader *loaders;
  HashTableAssetDestructor *destructors;
  HashTableAssetStore *asset_stores;
};

Assets *assets_new() {
  Assets *assets = memory_allocate(sizeof(Assets));
  assets->loaders = HashTableAssetLoader_create(16);
  assets->destructors = HashTableAssetDestructor_create(16);
  assets->asset_stores = HashTableAssetStore_create(16);
  return assets;
}

void *assets_load_asset(Assets *assets, char *asset_type, char *asset_path) {
  LOG_DEBUG("Loading asset %s of type %s", asset_path, asset_type);
  AssetLoader *loader = HashTableAssetLoader_get(assets->loaders, asset_type);
  if (!loader) {
    LOG_ERROR("No asset loader registered for asset type %s", asset_type);
    return NULL;
  }

  void *asset = loader->fn(asset_path);
  if (!asset) {
    LOG_ERROR("Could not load asset %s of type %s", asset_path, asset_type);
    return NULL;
  }

  AssetStore *asset_store =
      HashTableAssetStore_get(assets->asset_stores, asset_type);
  if (!asset_store) {
    LOG_DEBUG("No asset store found for assets of type %s, creating it",
              asset_type);
    HashTableAssetStore_set(
        assets->asset_stores, asset_type,
        asset_store_new(
            HashTableAssetDestructor_get(assets->destructors, asset_type)->fn));
    asset_store = HashTableAssetStore_get(assets->asset_stores, asset_type);
  }

  asset_store_set(asset_store, asset_path, asset);

  LOG_DEBUG("Asset %s of type %s successfully loaded", asset_path, asset_type);
  return asset;
}

void assets_clear(Assets *assets) {
  LOG_DEBUG("Clearing assets...");
  HashTableAssetStore_clear(assets->asset_stores);
}

void *assets_fetch_(Assets *assets, char *asset_type, char *asset_path) {
  LOG_DEBUG("Fetching asset %s of type %s", asset_path, asset_type);
  AssetStore *asset_store =
      HashTableAssetStore_get(assets->asset_stores, asset_type);

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

void assets_register_loader_(Assets *assets, char *asset_type,
                             AssetLoader *asset_loader,
                             AssetDestructor *asset_destructor) {
  HashTableAssetLoader_set(assets->loaders, asset_type, asset_loader);
  HashTableAssetDestructor_set(assets->destructors, asset_type,
                               asset_destructor);
}
bool assets_is_loader_registered_for_type_(const Assets *assets,
                                           const char *asset_type) {
  return HashTableAssetLoader_has(assets->loaders, asset_type);
}

void assets_remove_(Assets *assets, const char *asset_type,
                    const char *asset_path) {
  LOG_DEBUG("Removing asset %s of type %s", asset_path, asset_type);
  AssetStore *asset_store =
      HashTableAssetStore_get(assets->asset_stores, asset_type);
  asset_store_remove(asset_store, asset_path);
}

void assets_destroy(Assets *assets) {

  for (size_t asset_store_index = 0;
       asset_store_index < assets->asset_stores->capacity;
       asset_store_index++) {
    if (!assets->asset_stores->items[asset_store_index].key) {
      continue;
    }

    char *asset_type = assets->asset_stores->items[asset_store_index].key;
    AssetStore *asset_store =
        assets->asset_stores->items[asset_store_index].value;

    AssetDestructor *destructor =
        HashTableAssetDestructor_get(assets->destructors, asset_type);
    for (size_t asset_index = 0; asset_index < asset_store->assets->capacity;
         asset_index++) {
      if (!asset_store->assets->items[asset_index].key) {
        continue;
      }

      destructor->fn(asset_store->assets->items[asset_index].value);
    }
  }

  HashTableAssetStore_destroy(assets->asset_stores);
  HashTableAssetLoader_destroy(assets->loaders);
  HashTableAssetDestructor_destroy(assets->destructors);
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
