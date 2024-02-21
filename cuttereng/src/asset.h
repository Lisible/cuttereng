#ifndef CUTTERENG_ASSET_H
#define CUTTERENG_ASSET_H

#include "common.h"
#include "memory.h"

typedef void *(*AssetLoaderFn)(Allocator *allocator, const char *path);
typedef struct {
  AssetLoaderFn fn;
} AssetLoader;

typedef void (*AssetDestructorFn)(Allocator *allocator, void *asset);
typedef struct {
  AssetDestructorFn fn;
} AssetDestructor;

typedef struct Assets Assets;
Assets *assets_new(Allocator *allocator);
void assets_register_loader_(Assets *assets, char *asset_type,
                             AssetLoader *asset_loader,
                             AssetDestructor *asset_destructor);
bool assets_is_loader_registered_for_type_(const Assets *assets,
                                           const char *asset_type);

void assets_store_(Assets *assets, char *asset_type, char *asset_identifier,
                   void *asset);
void *assets_fetch_(Assets *assets, char *asset_type, char *asset_path);
void assets_destroy(Assets *assets);
void assets_remove_(Assets *assets, const char *asset_type,
                    const char *asset_path);
void assets_set_destructor_(Assets *assets, char *asset_type,
                            AssetDestructor *asset_destructor);
void assets_clear(Assets *assets);
void assets_list_subdirectory(const char *subdirectory);

/// Returns the effective path of an asset
/// @return The path, owned by the caller
char *asset_get_effective_path(Allocator *allocator, const char *path);
char *asset_read_file_to_string(Allocator *allocator, const char *path);

#define assets_store(assets, asset_type, asset_identifier, asset)              \
  assets_store_(assets, #asset_type, asset_identifier, asset)
#define assets_set_destructor(assets, asset_type, asset_destructor)            \
  assets_set_destructor_(assets, #asset_type, asset_destructor);
#define assets_remove(assets, asset_type, asset_path)                          \
  assets_remove_(assets, #asset_type, asset_path)
#define assets_fetch(assets, asset_type, asset_path)                           \
  (asset_type *)assets_fetch_(assets, #asset_type, asset_path)
#define assets_register_loader(assets, asset_type, asset_loader,               \
                               asset_destructor)                               \
  assets_register_loader_(assets, #asset_type, asset_loader, asset_destructor)
#define assets_is_loader_registered_for_type(assets, asset_type)               \
  assets_is_loader_registered_for_type_(assets, #asset_type)

#endif // CUTTERENG_ASSET_H
