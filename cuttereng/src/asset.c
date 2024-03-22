#include "asset.h"
#include "assert.h"
#include "filesystem.h"
#include "hash.h"
#include "log.h"
#include "memory.h"
#include <errno.h>
#include <string.h>

#define ASSETS_BASE_PATH "assets/"

DECL_STRING_HASH_TABLE(void *, HashTableAsset)
// FIXME there needs to be a destructor, this will leak on asset reload
DEF_STRING_HASH_TABLE(void *, HashTableAsset, HashTable_noop_destructor)

DECL_STRING_HASH_TABLE(AssetLoader *, HashTableAssetLoader)
DEF_STRING_HASH_TABLE(AssetLoader *, HashTableAssetLoader,
                      HashTable_noop_destructor)
DECL_STRING_HASH_TABLE(AssetDeinitializer *, HashTableAssetDeinitializer)
DEF_STRING_HASH_TABLE(AssetDeinitializer *, HashTableAssetDeinitializer,
                      HashTable_noop_destructor)

typedef struct {
  Allocator *allocator;
  char *assets;
  size_t asset_type_size;
  size_t asset_type_alignment;
  size_t capacity;
  size_t length;
} AssetStore;

AssetStore *AssetStore_new(Allocator *allocator, size_t asset_type_alignment,
                           size_t asset_type_size) {
  ASSERT(allocator != NULL);
  AssetStore *asset_store = allocator_allocate(allocator, sizeof(AssetStore));
  asset_store->allocator = allocator;
  asset_store->asset_type_size = asset_type_size;
  asset_store->asset_type_alignment = asset_type_alignment;
  asset_store->capacity = ASSET_STORE_CAPACITY;
  asset_store->length = 0;
  asset_store->assets = allocator_allocate_aligned(
      allocator, asset_store->asset_type_alignment,
      asset_store->asset_type_size * asset_store->capacity);
  if (!asset_store->assets) {
    LOG_ERROR("AssetStore asset array couldn't be allocated: \n\t%s\n",
              strerror(errno));
    goto cleanup_asset_store;
  }
  return asset_store;

cleanup_asset_store:
  allocator_free(allocator, asset_store);
  return NULL;
}

void AssetStore_destroy(AssetStore *asset_store) {
  allocator_free(asset_store->allocator, asset_store->assets);
  allocator_free(asset_store->allocator, asset_store);
}

AssetHandle AssetStore_store(AssetStore *asset_store, const void *asset) {
  ASSERT(asset_store != NULL);
  ASSERT(asset != NULL);

  if (asset_store->length == asset_store->capacity) {
    PANIC("AssetStore is full");
  }
  memcpy(
      &asset_store->assets[asset_store->asset_type_size * asset_store->length],
      asset, asset_store->asset_type_size);

  AssetHandle handle = asset_store->length;
  asset_store->length++;
  return handle;
}

void *AssetStore_get(AssetStore *asset_store, AssetHandle handle) {
  ASSERT(asset_store != NULL);
  return &asset_store->assets[handle * asset_store->asset_type_size];
}

void AssetStore_destructor(Allocator *allocator, void *asset_store) {
  ASSERT(allocator != NULL);
  ASSERT(asset_store != NULL);
  AssetStore_destroy(asset_store);
}

DECL_STRING_HASH_TABLE(AssetStore *, HashTableAssetStore)
DEF_STRING_HASH_TABLE(AssetStore *, HashTableAssetStore, AssetStore_destructor)

struct Assets {
  Allocator *allocator;
  HashTableAssetLoader *loaders;
  HashTableAssetDeinitializer *destructors;
  HashTableAssetStore *asset_stores;
};

Assets *assets_new(Allocator *allocator) {
  Assets *assets = allocator_allocate(allocator, sizeof(Assets));
  assets->allocator = allocator;
  assets->loaders = HashTableAssetLoader_create(allocator, 16);
  assets->destructors = HashTableAssetDeinitializer_create(allocator, 16);
  assets->asset_stores = HashTableAssetStore_create(allocator, 16);
  return assets;
}

bool assets_load_asset(Assets *assets, const char *asset_type,
                       size_t asset_type_alignment, size_t asset_type_size,
                       const char *asset_path, AssetHandle *out_asset_handle) {
  ASSERT(assets != NULL);
  ASSERT(asset_type != NULL);
  ASSERT(asset_path != NULL);
  LOG_DEBUG("Loading asset %s of type %s", asset_path, asset_type);
  AssetLoader *loader =
      HashTableAssetLoader_get(assets->loaders, asset_type, strlen(asset_type));
  if (!loader) {
    LOG_ERROR("No asset loader registered for asset type %s", asset_type);
    return false;
  }

  void *asset = loader->fn(assets->allocator, assets, asset_path);
  if (!asset) {
    LOG_ERROR("Could not load asset %s of type %s", asset_path, asset_type);
    return false;
  }

  *out_asset_handle = assets_store_(assets, asset_type, asset_type_alignment,
                                    asset_type_size, asset);
  allocator_free(assets->allocator, asset);
  return true;
}
AssetHandle assets_store_(Assets *assets, const char *asset_type,
                          size_t asset_type_alignment, size_t asset_type_size,
                          const void *asset) {
  ASSERT(assets != NULL);
  ASSERT(asset_type != NULL);
  ASSERT(asset != NULL);

  AssetStore *asset_store = HashTableAssetStore_get(
      assets->asset_stores, asset_type, strlen(asset_type));
  if (!asset_store) {
    LOG_DEBUG("No asset store found for assets of type %s, creating it",
              asset_type);
    HashTableAssetStore_set_strkey(assets->asset_stores, (char *)asset_type,
                                   AssetStore_new(assets->allocator,
                                                  asset_type_alignment,
                                                  asset_type_size));
    asset_store = HashTableAssetStore_get(assets->asset_stores, asset_type,
                                          strlen(asset_type));
  }

  AssetHandle asset_handle = AssetStore_store(asset_store, asset);
  LOG_DEBUG("Asset %d of type %s successfully stored", asset_handle,
            asset_type);
  return asset_handle;
}

void assets_clear(Assets *assets) {
  ASSERT(assets != NULL);
  LOG_DEBUG("Clearing assets...");
  HashTableAssetStore_clear(assets->asset_stores);
}

bool assets_load_(Assets *assets, const char *asset_type,
                  size_t asset_type_alignment, size_t asset_type_size,
                  const char *asset_path, AssetHandle *out_asset_handle) {
  ASSERT(assets != NULL);
  ASSERT(asset_type != NULL);
  ASSERT(asset_path != NULL);
  LOG_DEBUG("Loading asset %s of type %s", asset_path, asset_type);
  AssetStore *asset_store = HashTableAssetStore_get(
      assets->asset_stores, asset_type, strlen(asset_type));
  if (!asset_store) {
    LOG_DEBUG("No asset store found for assets of type %s, creating it",
              asset_type);
    HashTableAssetStore_set_strkey(assets->asset_stores, (char *)asset_type,
                                   AssetStore_new(assets->allocator,
                                                  asset_type_alignment,
                                                  asset_type_size));
    asset_store = HashTableAssetStore_get(assets->asset_stores, asset_type,
                                          strlen(asset_type));
  }

  return assets_load_asset(assets, asset_type, asset_type_alignment,
                           asset_type_size, asset_path, out_asset_handle);
}
void *assets_get_(Assets *assets, const char *asset_type,
                  AssetHandle asset_handle) {
  ASSERT(assets != NULL);
  ASSERT(asset_type != NULL);
  AssetStore *asset_store = HashTableAssetStore_get(
      assets->asset_stores, asset_type, strlen(asset_type));
  if (!asset_store) {
    return NULL;
  }

  return AssetStore_get(asset_store, asset_handle);
}

void assets_register_asset_type_(Assets *assets, char *asset_type,
                                 AssetLoader *asset_loader,
                                 AssetDeinitializer *asset_deinitializer) {
  ASSERT(assets != NULL);
  ASSERT(asset_type != NULL);
  ASSERT(asset_loader != NULL);
  ASSERT(asset_deinitializer != NULL);
  HashTableAssetLoader_set_strkey(assets->loaders, asset_type, asset_loader);
  HashTableAssetDeinitializer_set_strkey(assets->destructors, asset_type,
                                         asset_deinitializer);
}
bool assets_is_loader_registered_for_type_(const Assets *assets,
                                           const char *asset_type) {
  ASSERT(assets != NULL);
  ASSERT(asset_type != NULL);
  return HashTableAssetLoader_has(assets->loaders, asset_type,
                                  strlen(asset_type));
}

void assets_set_deinitializer_(Assets *assets, char *asset_type,
                               AssetDeinitializer *asset_deinitializer) {
  ASSERT(assets != NULL);
  ASSERT(asset_type != NULL);
  ASSERT(asset_deinitializer != NULL);
  HashTableAssetDeinitializer_set_strkey(assets->destructors, asset_type,
                                         asset_deinitializer);
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
    AssetDeinitializer *deinitializer = HashTableAssetDeinitializer_get(
        assets->destructors, asset_type, strlen(asset_type));
    for (size_t i = 0; i < asset_store->length; i++) {
      deinitializer->fn(asset_store->allocator,
                        &asset_store->assets[asset_store->asset_type_size * i]);
    }
  }

  HashTableAssetStore_destroy(assets->asset_stores);
  HashTableAssetLoader_destroy(assets->loaders);
  HashTableAssetDeinitializer_destroy(assets->destructors);
  allocator_free(assets->allocator, assets);
}

char *asset_get_effective_path(Allocator *allocator, const char *path) {
  ASSERT(allocator != NULL);
  ASSERT(path != NULL);
  size_t effective_path_length = strlen(ASSETS_BASE_PATH) + strlen(path) + 1;
  char *effective_path = allocator_allocate(allocator, effective_path_length);
  memset(effective_path, 0, effective_path_length);
  strcat(effective_path, ASSETS_BASE_PATH);
  strcat(effective_path, path);
  return effective_path;
}
char *asset_read_file_to_string(Allocator *allocator, const char *path,
                                size_t *out_size) {
  char *effective_path = asset_get_effective_path(allocator, path);
  char *result =
      filesystem_read_file_to_string(allocator, effective_path, out_size);
  allocator_free(allocator, effective_path);
  return result;
}
