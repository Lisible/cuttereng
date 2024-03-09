#ifndef CUTTERENG_RENDERER_SHADER_H
#define CUTTERENG_RENDERER_SHADER_H

#include "../asset.h"
#include "../common.h"
#include "../memory.h"
#include <webgpu/webgpu.h>

typedef struct {
  char *source;
} Shader;

void *shader_asset_loader_fn(Allocator *allocator, Assets *assets,
                             const char *path);
extern AssetLoader shader_asset_loader;
void shader_asset_deinitializer_fn(Allocator *allocator, void *asset);
extern AssetDeinitializer shader_asset_deinitializer;

WGPUShaderModule shader_create_wgpu_shader_module(WGPUDevice device,
                                                  const char *label,
                                                  const char *shader_source);

#endif // CUTTERENG_RENDERER_SHADER_H
