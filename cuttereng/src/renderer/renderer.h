#ifndef CUTTERENG_RENDERER_RENDERER_H
#define CUTTERENG_RENDERER_RENDERER_H

#include "../asset.h"
#include "../camera.h"
#include "../hash.h"
#include "../math/matrix.h"
#include "../memory.h"
#include "../transform.h"
#include "mesh.h"
#include <SDL.h>
#include <webgpu/webgpu.h>

DECL_HASH_TABLE(WGPURenderPipeline, HashTableRenderPipeline)
DECL_HASH_TABLE(WGPUShaderModule, HashTableShaderModule)

typedef struct {
  mat4 projection_from_view;
  float current_time_secs;
  float _pad[15];
} CommonUniforms;

typedef struct {
  float world_from_local[16];
} MeshUniforms;

typedef struct {
  WGPUInstance wgpu_instance;
  WGPUAdapter wgpu_adapter;
  WGPUSurface wgpu_surface;
  WGPUDevice wgpu_device;
  WGPUTextureFormat wgpu_render_surface_texture_format;
  WGPUTextureFormat depth_texture_format;
} RendererContext;

typedef struct {
  WGPUTexture depth_texture;

  HashTableRenderPipeline *pipelines;
  HashTableShaderModule *shader_modules;

  CommonUniforms common_uniforms;
  WGPUBuffer common_uniforms_buffer;
  WGPUBindGroupLayout common_uniforms_bind_group_layout;
  WGPUBindGroup common_uniforms_bind_group;

  MeshUniforms mesh_uniforms;
  WGPUBuffer mesh_uniforms_buffer;
  WGPUBindGroupLayout mesh_uniforms_bind_group_layout;
  WGPUBindGroup mesh_uniforms_bind_group;
} RendererResources;

typedef struct {
  Allocator *allocator;
  RendererContext ctx;
  RendererResources resources;

} Renderer;

Renderer *renderer_new(Allocator *allocator, SDL_Window *window, Assets *assets,
                       float current_time_secs);
void renderer_destroy(Renderer *renderer);
void renderer_render(Allocator *frame_allocator, Renderer *renderer,
                     Assets *assets, float current_time_secs);

void renderer_initialize_for_window(Renderer *renderer, SDL_Window *window);

#endif // CUTTERENG_RENDERER_RENDERER_H
