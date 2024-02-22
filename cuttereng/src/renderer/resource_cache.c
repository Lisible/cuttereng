#include "resource_cache.h"
#include "../assert.h"
#include "../bitset.h"
#include <string.h>

void ResourceCaches_init(ResourceCaches *caches) { ASSERT(caches != NULL); }
void ResourceCaches_clear(ResourceCaches *caches) {
  ASSERT(caches != NULL);
  for (size_t texture_index = 0; texture_index < RENDERER_TEXTURE_CACHE_SIZE;
       texture_index++) {
    if (BITTEST(caches->set_textures, texture_index)) {
      wgpuTextureDestroy(caches->textures[texture_index]);
      wgpuTextureRelease(caches->textures[texture_index]);
    }
  }

  for (size_t shader_module_index = 0;
       shader_module_index < RENDERER_SHADER_MODULE_CACHE_SIZE;
       shader_module_index++) {
    if (BITTEST(caches->set_shader_modules, shader_module_index)) {
      wgpuShaderModuleRelease(caches->shader_modules[shader_module_index]);
    }
  }

  for (size_t material_index = 0; material_index < RENDERER_MATERIAL_CACHE_SIZE;
       material_index++) {
    if (BITTEST(caches->set_materials, material_index)) {
      gpu_material_deinit(caches->materials[material_index]);
    }
  }
  for (size_t mesh_index = 0; mesh_index < RENDERER_MESH_CACHE_SIZE;
       mesh_index++) {
    if (BITTEST(caches->set_meshes, mesh_index)) {
      gpu_mesh_deinit(caches->meshes[mesh_index]);
    }
  }

  memset(caches->set_textures, 0, 16 * sizeof(u64));
  memset(caches->set_shader_modules, 0, 16 * sizeof(u64));
  memset(caches->set_materials, 0, 16 * sizeof(u64));
  memset(caches->set_meshes, 0, 16 * sizeof(u64));
}
void ResourceCaches_set_texture(ResourceCaches *caches, AssetHandle handle,
                                WGPUTexture texture) {
  ASSERT(caches != NULL);
  ASSERT(handle < RENDERER_TEXTURE_CACHE_SIZE);
  BITSET(caches->set_textures, handle);
  caches->textures[handle] = texture;
}
WGPUTexture ResourceCaches_get_texture(ResourceCaches *caches,
                                       AssetHandle handle) {
  ASSERT(caches != NULL);
  ASSERT(BITTEST(caches->set_textures, handle));
  return caches->textures[handle];
}
bool ResourceCaches_has_texture(ResourceCaches *caches, AssetHandle handle) {
  ASSERT(caches != NULL);
  return BITTEST(caches->set_textures, handle);
}
void ResourceCaches_set_material(ResourceCaches *caches, AssetHandle handle,
                                 GPUMaterial *material) {
  ASSERT(caches != NULL);
  ASSERT(handle < RENDERER_MATERIAL_CACHE_SIZE);
  BITSET(caches->set_materials, handle);
  caches->materials[handle] = material;
}
GPUMaterial *ResourceCaches_get_material(ResourceCaches *caches,
                                         AssetHandle handle) {
  ASSERT(caches != NULL);
  ASSERT(BITTEST(caches->set_materials, handle));
  return caches->materials[handle];
}
bool ResourceCaches_has_material(ResourceCaches *caches, AssetHandle handle) {
  ASSERT(caches != NULL);
  return BITTEST(caches->set_materials, handle);
}

void ResourceCaches_set_mesh(ResourceCaches *caches, AssetHandle handle,
                             GPUMesh *mesh) {
  ASSERT(caches != NULL);
  ASSERT(handle < RENDERER_MESH_CACHE_SIZE);
  BITSET(caches->set_meshes, handle);
  caches->meshes[handle] = mesh;
}
GPUMesh *ResourceCaches_get_mesh(ResourceCaches *caches, AssetHandle handle) {
  ASSERT(caches != NULL);
  ASSERT(BITTEST(caches->set_meshes, handle));
  return caches->meshes[handle];
}
bool ResourceCaches_has_mesh(ResourceCaches *caches, AssetHandle handle) {
  ASSERT(caches != NULL);
  return BITTEST(caches->set_meshes, handle);
}
void ResourceCaches_set_shader_module(ResourceCaches *caches,
                                      AssetHandle handle,
                                      WGPUShaderModule shader_module) {
  ASSERT(caches != NULL);
  ASSERT(handle < RENDERER_SHADER_MODULE_CACHE_SIZE);
  BITSET(caches->set_shader_modules, handle);
  LOG_DEBUG("set handle: %ld", handle);
  caches->shader_modules[handle] = shader_module;
}
WGPUShaderModule ResourceCaches_get_shader_module(ResourceCaches *caches,
                                                  AssetHandle handle) {
  ASSERT(caches != NULL);
  LOG_DEBUG("handle: %ld", handle);
  ASSERT(BITTEST(caches->set_shader_modules, handle));
  return caches->shader_modules[handle];
}
