#include "shader.h"
#include "../asset.h"
#include "../memory.h"

void *shader_asset_loader(const char *path) {
  Shader *shader_asset = memory_allocate(sizeof(Shader));
  shader_asset->source = asset_read_file_to_string(path);
  return shader_asset;
}
void shader_asset_destructor(void *asset) {
  Shader *shader_asset = asset;
  memory_free(shader_asset->source);
  memory_free(shader_asset);
}

WGPUShaderModule shader_create_wgpu_shader_module(WGPUDevice device,
                                                  const char *label,
                                                  const char *shader_source) {
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
