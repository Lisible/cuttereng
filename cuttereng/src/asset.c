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
  Allocator *allocator;
  HashTableAsset *assets;
} AssetStore;

AssetStore *asset_store_new(Allocator *allocator) {
  AssetStore *asset_store = allocator_allocate(allocator, sizeof(AssetStore));
  asset_store->allocator = allocator;
  asset_store->assets = HashTableAsset_create(allocator, 16);
  return asset_store;
}

void asset_store_set(AssetStore *asset_store, char *key, void *asset) {
  ASSERT(asset_store != NULL);
  ASSERT(key != NULL);
  ASSERT(asset != NULL);
  HashTableAsset_set(asset_store->assets, key, asset);
}

void *asset_store_get(AssetStore *asset_store, const char *key) {
  ASSERT(asset_store != NULL);
  ASSERT(key != NULL);
  return HashTableAsset_get(asset_store->assets, key);
}

void asset_store_remove(AssetStore *asset_store, const char *key) {
  ASSERT(asset_store != NULL);
  ASSERT(key != NULL);
  HashTableAsset_remove(asset_store->assets, key);
}

void asset_store_destroy(Allocator *allocator, AssetStore *asset_store) {
  ASSERT(asset_store != NULL);
  HashTableAsset_destroy(asset_store->assets);
  allocator_free(asset_store->allocator, asset_store);
}

DECL_HASH_TABLE(AssetLoader, HashTableAssetLoader);
DEF_HASH_TABLE(AssetLoader, HashTableAssetLoader, HashTable_noop_destructor);
DECL_HASH_TABLE(AssetDestructor, HashTableAssetDestructor);
DEF_HASH_TABLE(AssetDestructor, HashTableAssetDestructor,
               HashTable_noop_destructor);
DECL_HASH_TABLE(AssetStore, HashTableAssetStore);
DEF_HASH_TABLE(AssetStore, HashTableAssetStore, asset_store_destroy);

struct Assets {
  Allocator *allocator;
  HashTableAssetLoader *loaders;
  HashTableAssetDestructor *destructors;
  HashTableAssetStore *asset_stores;
};

Assets *assets_new(Allocator *allocator) {
  Assets *assets = allocator_allocate(allocator, sizeof(Assets));
  assets->allocator = allocator;
  assets->loaders = HashTableAssetLoader_create(allocator, 16);
  assets->destructors = HashTableAssetDestructor_create(allocator, 16);
  assets->asset_stores = HashTableAssetStore_create(allocator, 16);
  return assets;
}

void *assets_load_asset(Assets *assets, char *asset_type, char *asset_path) {
  ASSERT(assets != NULL);
  ASSERT(asset_type != NULL);
  ASSERT(asset_path != NULL);
  LOG_DEBUG("Loading asset %s of type %s", asset_path, asset_type);
  AssetLoader *loader = HashTableAssetLoader_get(assets->loaders, asset_type);
  if (!loader) {
    LOG_ERROR("No asset loader registered for asset type %s", asset_type);
    return NULL;
  }

  void *asset = loader->fn(assets->allocator, asset_path);
  if (!asset) {
    LOG_ERROR("Could not load asset %s of type %s", asset_path, asset_type);
    return NULL;
  }

  AssetStore *asset_store =
      HashTableAssetStore_get(assets->asset_stores, asset_type);
  if (!asset_store) {
    LOG_DEBUG("No asset store found for assets of type %s, creating it",
              asset_type);
    HashTableAssetStore_set(assets->asset_stores, asset_type,
                            asset_store_new(assets->allocator));
    asset_store = HashTableAssetStore_get(assets->asset_stores, asset_type);
  }

  asset_store_set(asset_store, asset_path, asset);

  LOG_DEBUG("Asset %s of type %s successfully loaded", asset_path, asset_type);
  return asset;
}

void assets_clear(Assets *assets) {
  ASSERT(assets != NULL);
  LOG_DEBUG("Clearing assets...");
  HashTableAssetStore_clear(assets->asset_stores);
}

void *assets_fetch_(Assets *assets, char *asset_type, char *asset_path) {
  ASSERT(assets != NULL);
  ASSERT(asset_type != NULL);
  ASSERT(asset_path != NULL);
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
  ASSERT(assets != NULL);
  ASSERT(asset_type != NULL);
  ASSERT(asset_loader != NULL);
  ASSERT(asset_destructor != NULL);
  HashTableAssetLoader_set(assets->loaders, asset_type, asset_loader);
  HashTableAssetDestructor_set(assets->destructors, asset_type,
                               asset_destructor);
}
bool assets_is_loader_registered_for_type_(const Assets *assets,
                                           const char *asset_type) {
  ASSERT(assets != NULL);
  ASSERT(asset_type != NULL);
  return HashTableAssetLoader_has(assets->loaders, asset_type);
}

void assets_remove_(Assets *assets, const char *asset_type,
                    const char *asset_path) {
  ASSERT(assets != NULL);
  ASSERT(asset_type != NULL);
  ASSERT(asset_path != NULL);
  LOG_DEBUG("Removing asset %s of type %s", asset_path, asset_type);
  AssetStore *asset_store =
      HashTableAssetStore_get(assets->asset_stores, asset_type);
  asset_store_remove(asset_store, asset_path);
}

void assets_destroy(Assets *assets) {
  ASSERT(assets != NULL);

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

      destructor->fn(assets->allocator,
                     asset_store->assets->items[asset_index].value);
    }
  }

  HashTableAssetStore_destroy(assets->asset_stores);
  HashTableAssetLoader_destroy(assets->loaders);
  HashTableAssetDestructor_destroy(assets->destructors);
  allocator_free(assets->allocator, assets);
}
char *asset_read_file_to_string(Allocator *allocator, const char *path) {
  ASSERT(path != NULL);
  size_t effective_path_length = strlen(ASSETS_BASE_PATH) + strlen(path) + 1;
  char *effective_path = allocator_allocate(allocator, effective_path_length);
  memset(effective_path, 0, effective_path_length);
  strcat(effective_path, ASSETS_BASE_PATH);
  strcat(effective_path, path);
  return filesystem_read_file_to_string(allocator, effective_path);
}
