#ifndef CUTTERENG_ASSET_H
#define CUTTERENG_ASSET_H

#include "common.h"

typedef void *(*AssetLoader)(const char *path);
typedef void (*AssetDestructor)(void *asset);

typedef struct Assets Assets;
Assets *assets_new();
void assets_register_loader_(Assets *assets, const char *asset_type,
                             AssetLoader asset_loader,
                             AssetDestructor asset_destructor);
bool assets_is_loader_registered_for_type_(const Assets *assets,
                                           const char *asset_type);
void *assets_fetch_(Assets *assets, const char *asset_type,
                    const char *asset_path);
void assets_destroy(Assets *assets);

#define assets_fetch(assets, asset_type, asset_path)                           \
  (asset_type *)assets_fetch_(assets, #asset_type, asset_path)
#define assets_register_loader(assets, asset_type, asset_loader,               \
                               asset_destructor)                               \
  assets_register_loader_(assets, #asset_type, asset_loader, asset_destructor)
#define assets_is_loader_registered_for_type(assets, asset_type)               \
  assets_is_loader_registered_for_type_(assets, #asset_type)

#endif // CUTTERENG_ASSET_H
