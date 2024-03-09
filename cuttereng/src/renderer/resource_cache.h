#ifndef CUTTERENG_RENDERER_RESOURCE_CACHE_H
#define CUTTERENG_RENDERER_RESOURCE_CACHE_H

#include "material.h"
#include "mesh.h"
#include <webgpu/webgpu.h>

#define RENDERER_TEXTURE_CACHE_SIZE 1024
#define RENDERER_MATERIAL_CACHE_SIZE 1024
#define RENDERER_MESH_CACHE_SIZE 1024
#define RENDERER_SHADER_MODULE_CACHE_SIZE 1024

struct ResourceCaches {
  WGPUTexture textures[RENDERER_TEXTURE_CACHE_SIZE];
  WGPUShaderModule shader_modules[RENDERER_SHADER_MODULE_CACHE_SIZE];
  GPUMaterial *materials[RENDERER_MATERIAL_CACHE_SIZE];
  GPUMesh *meshes[RENDERER_MESH_CACHE_SIZE];
  u8 set_textures[128];
  u8 set_shader_modules[128];
  u8 set_materials[128];
  u8 set_meshes[128];
};
typedef struct ResourceCaches ResourceCaches;

void ResourceCaches_init(ResourceCaches *caches);
void ResourceCaches_clear(Allocator *allocator, ResourceCaches *caches);

void ResourceCaches_set_texture(ResourceCaches *caches, AssetHandle handle,
                                WGPUTexture texture);
WGPUTexture ResourceCaches_get_texture(ResourceCaches *caches,
                                       AssetHandle handle);
bool ResourceCaches_has_texture(ResourceCaches *caches, AssetHandle handle);

void ResourceCaches_set_material(ResourceCaches *caches, AssetHandle handle,
                                 GPUMaterial *material);
GPUMaterial *ResourceCaches_get_material(ResourceCaches *caches,
                                         AssetHandle handle);
bool ResourceCaches_has_material(ResourceCaches *caches, AssetHandle handle);

void ResourceCaches_set_mesh(ResourceCaches *caches, AssetHandle handle,
                             GPUMesh *mesh);
GPUMesh *ResourceCaches_get_mesh(ResourceCaches *caches, AssetHandle handle);
bool ResourceCaches_has_mesh(ResourceCaches *caches, AssetHandle handle);

void ResourceCaches_set_shader_module(ResourceCaches *caches,
                                      AssetHandle handle,
                                      WGPUShaderModule shader_module);
WGPUShaderModule ResourceCaches_get_shader_module(ResourceCaches *caches,
                                                  AssetHandle handle);

#endif // CUTTERENG_RENDERER_RESOURCE_CACHE_H
