#include "material.h"
#include "../assert.h"
#include "../image.h"
#include "../json.h"
#include "renderer.h"
#include "webgpu/webgpu.h"
#include <string.h>

AssetLoader material_loader = {.fn = material_loader_fn};
void *material_loader_fn(Allocator *allocator, Assets *assets,
                         const char *path) {
  ASSERT(allocator != NULL);
  ASSERT(path != NULL);
  Material *material = allocator_allocate(allocator, sizeof(Material));
  char *material_file_content =
      asset_read_file_to_string(allocator, path, NULL);
  if (!material_file_content) {
    LOG_ERROR("Couldn't read material file: %s", path);
    goto err;
  }

  Json *material_json = json_parse_from_str(allocator, material_file_content);
  if (!material_json) {
    LOG_ERROR("Couldn't parse material file: %s", path);
    goto cleanup_file_content_str;
  }

  if (material_json->type != JSON_OBJECT) {
    LOG_ERROR("Material file %s is not a JSON object", path);
    goto cleanup_material_json;
  }
  Json *base_color_json = json_object_get(material_json->object, "base_color");
  if (!base_color_json) {
    LOG_ERROR("base_color property missing from material file: %s", path);
    goto cleanup_material_json;
  }

  if (base_color_json->type != JSON_STRING) {
    LOG_ERROR("base_color property of material file %s is not a string", path);
    goto cleanup_material_json;
  }

  AssetHandle base_color_handle;
  if (!assets_load(assets, Image, base_color_json->string,
                   &base_color_handle)) {
    LOG_ERROR("Couldn't load base color texture");
  }

  Json *normal_json = json_object_get(material_json->object, "normal");
  if (!normal_json) {
    LOG_ERROR("normal property missing from material file: %s", path);
    goto cleanup_material_json;
  }

  if (normal_json->type != JSON_STRING) {
    LOG_ERROR("normal property of material file %s is not a string", path);
    goto cleanup_material_json;
  }

  AssetHandle normal_handle;
  if (!assets_load(assets, Image, normal_json->string, &normal_handle)) {
    LOG_ERROR("Couldn't load normal texture");
  }

  material->base_color = base_color_handle;
  material->normal = normal_handle;
  json_destroy(allocator, material_json);
  allocator_free(allocator, material_file_content);
  return material;

cleanup_material_json:
  json_destroy(allocator, material_json);
cleanup_file_content_str:
  allocator_free(allocator, material_file_content);
err:
  allocator_free(allocator, material);
  return NULL;
}

AssetDeinitializer material_deinitializer = {.fn = material_deinitializer_fn};
void material_deinitializer_fn(Allocator *allocator, void *asset) {
  (void)allocator;
  Material *material = asset;
  (void)material;
}

GPUMaterial *gpu_material_create(Allocator *allocator, ResourceCaches *caches,
                                 WGPUDevice device,
                                 WGPUBindGroupLayout material_bind_group_layout,
                                 Material *material) {
  GPUMaterial *gpu_material =
      allocator_allocate(allocator, sizeof(GPUMaterial));
  WGPUTexture base_color_texture =
      ResourceCaches_get_texture(caches, material->base_color);
  if (!base_color_texture) {
    allocator_free(allocator, gpu_material);
    return NULL;
  }

  WGPUTexture normal_texture =
      ResourceCaches_get_texture(caches, material->normal);
  if (!normal_texture) {
    allocator_free(allocator, gpu_material);
    return NULL;
  }

  WGPUTextureView base_color_texture_view = wgpuTextureCreateView(
      base_color_texture, &(const WGPUTextureViewDescriptor){
                              .aspect = WGPUTextureAspect_All,
                              .arrayLayerCount = 1,
                              .baseArrayLayer = 0,
                              .mipLevelCount = 1,
                              .baseMipLevel = 0,
                              .dimension = WGPUTextureViewDimension_2D,
                              .format = WGPUTextureFormat_RGBA8UnormSrgb});
  WGPUSampler base_color_texture_sampler = wgpuDeviceCreateSampler(
      device, &(const WGPUSamplerDescriptor){
                  .addressModeU = WGPUAddressMode_ClampToEdge,
                  .addressModeV = WGPUAddressMode_ClampToEdge,
                  .addressModeW = WGPUAddressMode_ClampToEdge,
                  .magFilter = WGPUFilterMode_Nearest,
                  .minFilter = WGPUFilterMode_Nearest,
                  .lodMinClamp = 0.0f,
                  .lodMaxClamp = 1.0f,
                  .maxAnisotropy = 1});
  WGPUTextureView normal_texture_view = wgpuTextureCreateView(
      normal_texture, &(const WGPUTextureViewDescriptor){
                          .aspect = WGPUTextureAspect_All,
                          .arrayLayerCount = 1,
                          .baseArrayLayer = 0,
                          .mipLevelCount = 1,
                          .baseMipLevel = 0,
                          .dimension = WGPUTextureViewDimension_2D,
                          .format = WGPUTextureFormat_RGBA8UnormSrgb});
  WGPUSampler normal_texture_sampler = wgpuDeviceCreateSampler(
      device, &(const WGPUSamplerDescriptor){
                  .addressModeU = WGPUAddressMode_ClampToEdge,
                  .addressModeV = WGPUAddressMode_ClampToEdge,
                  .addressModeW = WGPUAddressMode_ClampToEdge,
                  .magFilter = WGPUFilterMode_Nearest,
                  .minFilter = WGPUFilterMode_Nearest,
                  .lodMinClamp = 0.0f,
                  .lodMaxClamp = 1.0f,
                  .maxAnisotropy = 1});

  gpu_material->base_color_texture_view = base_color_texture_view;
  gpu_material->base_color_texture_sampler = base_color_texture_sampler;
  gpu_material->normal_texture_view = normal_texture_view;
  gpu_material->normal_texture_sampler = normal_texture_sampler;

  gpu_material->bind_group = wgpuDeviceCreateBindGroup(
      device,
      &(const WGPUBindGroupDescriptor){
          .entryCount = 4,
          .entries =
              (const WGPUBindGroupEntry[]){
                  (const WGPUBindGroupEntry){
                      .binding = 0, .textureView = base_color_texture_view},
                  (const WGPUBindGroupEntry){
                      .binding = 1, .sampler = base_color_texture_sampler},
                  (const WGPUBindGroupEntry){
                      .binding = 2, .textureView = normal_texture_view},
                  (const WGPUBindGroupEntry){
                      .binding = 3, .sampler = normal_texture_sampler}},
          .layout = material_bind_group_layout});

  return gpu_material;
}

void gpu_material_deinit(GPUMaterial *material) {
  wgpuTextureViewRelease(material->base_color_texture_view);
  wgpuSamplerRelease(material->base_color_texture_sampler);
  wgpuTextureViewRelease(material->normal_texture_view);
  wgpuSamplerRelease(material->normal_texture_sampler);
  wgpuBindGroupRelease(material->bind_group);
}

void gpu_material_destroy(Allocator *allocator, GPUMaterial *material) {
  gpu_material_deinit(material);
  allocator_free(allocator, material);
}
