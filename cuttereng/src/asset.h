#ifndef CUTTERENG_ASSET_H
#define CUTTERENG_ASSET_H

#include "memory.h"
#include <stdalign.h>

#define ASSET_STORE_CAPACITY 1024
typedef size_t AssetHandle;

typedef struct Assets Assets;
typedef void *(*AssetLoaderFn)(Allocator *allocator, Assets *assets,
                               const char *path);
typedef struct {
  AssetLoaderFn fn;
} AssetLoader;

typedef void (*AssetDeinitFn)(Allocator *allocator, void *asset);
typedef struct {
  AssetDeinitFn fn;
} AssetDeinitializer;

Assets *assets_new(Allocator *allocator);
void assets_register_asset_type_(Assets *assets, char *asset_type,
                                 AssetLoader *asset_loader,
                                 AssetDeinitializer *asset_deinitializer);
bool assets_is_loader_registered_for_type_(const Assets *assets,
                                           const char *asset_type);

AssetHandle assets_store_(Assets *assets, const char *asset_type,
                          size_t asset_type_alignment, size_t asset_type_size,
                          const void *asset);
bool assets_load_(Assets *assets, const char *asset_type,
                  size_t asset_type_alignment, size_t asset_type_size,
                  const char *asset_path, AssetHandle *out_asset_handle);
void *assets_get_(Assets *assets, const char *asset_type,
                  AssetHandle asset_handle);
void assets_destroy(Assets *assets);
void assets_set_deinitializer_(Assets *assets, char *asset_type,
                               AssetDeinitializer *asset_deinitializer);
void assets_clear(Assets *assets);
void assets_list_subdirectory(const char *subdirectory);

/// Returns the effective path of an asset
/// @return The path, owned by the caller
char *asset_get_effective_path(Allocator *allocator, const char *path);
char *asset_read_file_to_string(Allocator *allocator, const char *path,
                                size_t *out_size);

#define assets_store(assets, asset_type, asset_identifier, asset)              \
  assets_store_(assets, #asset_type, alignof(asset_type), sizeof(asset_type),  \
                asset)
#define assets_set_deinitializer(assets, asset_type, asset_deinitializer)      \
  assets_set_deinitializer_(assets, #asset_type, asset_deinitializer);
#define assets_remove(assets, asset_type, asset_path)                          \
  assets_remove_(assets, #asset_type, asset_path)
#define assets_load(assets, asset_type, asset_path, out_asset_handle)          \
  assets_load_(assets, #asset_type, alignof(asset_type), sizeof(asset_type),   \
               asset_path, out_asset_handle)
#define assets_get(assets, asset_type, asset_handle)                           \
  (asset_type *)assets_get_(assets, #asset_type, asset_handle)
#define assets_register_asset_type(assets, asset_type, asset_loader,           \
                                   asset_destructor)                           \
  assets_register_asset_type_(assets, #asset_type, asset_loader,               \
                              asset_destructor)
#define assets_is_loader_registered_for_type(assets, asset_type)               \
  assets_is_loader_registered_for_type_(assets, #asset_type)

#endif // CUTTERENG_ASSET_H
