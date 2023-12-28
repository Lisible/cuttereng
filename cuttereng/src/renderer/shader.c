#include "shader.h"
#include "../assert.h"
#include "../asset.h"
#include "../memory.h"

AssetLoader shader_asset_loader = {.fn = shader_asset_loader_fn};
void *shader_asset_loader_fn(const char *path) {
  ASSERT(path != NULL);
  Shader *shader_asset = memory_allocate(sizeof(Shader));
  shader_asset->source = asset_read_file_to_string(path);
  return shader_asset;
}
AssetDestructor shader_asset_destructor = {.fn = shader_asset_destructor_fn};
void shader_asset_destructor_fn(void *asset) {
  Shader *shader_asset = asset;
  memory_free(shader_asset->source);
  memory_free(shader_asset);
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
