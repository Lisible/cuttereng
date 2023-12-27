#ifndef CUTTERENG_RENDERER_SHADER_H
#define CUTTERENG_RENDERER_SHADER_H

#include "../common.h"
#include <webgpu/webgpu.h>

typedef struct {
  char *source;
} Shader;

void *shader_asset_loader(const char *path);
void shader_asset_destructor(void *asset);
WGPUShaderModule shader_create_wgpu_shader_module(WGPUDevice device,
                                                  const char *label,
                                                  const char *shader_source);

#endif // CUTTERENG_RENDERER_SHADER_H
