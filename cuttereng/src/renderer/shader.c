#include "shader.h"
#include "../assert.h"
#include "../asset.h"

AssetLoader shader_asset_loader = {.fn = shader_asset_loader_fn};
void *shader_asset_loader_fn(Allocator *allocator, const char *path) {
  ASSERT(path != NULL);
  Shader *shader_asset = allocator_allocate(allocator, sizeof(Shader));
  shader_asset->source = asset_read_file_to_string(allocator, path);
  if (!shader_asset->source) {
    goto cleanup_shader;
  }

  return shader_asset;

cleanup_shader:
  allocator_free(allocator, shader_asset);
  return NULL;
}
AssetDestructor shader_asset_destructor = {.fn = shader_asset_destructor_fn};
void shader_asset_destructor_fn(Allocator *allocator, void *asset) {
  Shader *shader_asset = asset;
  allocator_free(allocator, shader_asset->source);
  allocator_free(allocator, shader_asset);
}

WGPUShaderModule shader_create_wgpu_shader_module(WGPUDevice device,
                                                  const char *label,
                                                  const char *shader_source) {
  ASSERT(device != NULL);
  ASSERT(label != NULL);
  ASSERT(shader_source != NULL);
  WGPUShaderModuleWGSLDescriptor shader_module_wgsl_descriptor = {
      .chain =
          (WGPUChainedStruct){.next = NULL,
                              .sType = WGPUSType_ShaderModuleWGSLDescriptor},
      .code = shader_source};

  return wgpuDeviceCreateShaderModule(
      device, &(const WGPUShaderModuleDescriptor){
                  .label = label,
                  .nextInChain = &shader_module_wgsl_descriptor.chain,
                  .hintCount = 0,
                  .hints = NULL});
}
