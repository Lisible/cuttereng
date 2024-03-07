#ifndef CUTTERENG_GRAPHICS_MESH_INSTANCE_H
#define CUTTERENG_GRAPHICS_MESH_INSTANCE_H

#include "../asset.h"

typedef struct {
  AssetHandle mesh_handle;
  AssetHandle material_handle;
} MeshInstance;

#endif // CUTTERENG_GRAPHICS_MESH_INSTANCE_H
