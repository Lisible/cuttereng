#ifndef CUTTERENG_RENDERER_RENDERER_H
#define CUTTERENG_RENDERER_RENDERER_H

#include "../asset.h"
#include "../camera.h"
#include "../math/matrix.h"
#include "../transform.h"
#include "mesh.h"
#include <SDL.h>
#include <webgpu/webgpu.h>

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
  WGPURenderPipeline pipeline;

  CommonUniforms common_uniforms;
  WGPUBuffer common_uniforms_buffer;
  WGPUBindGroupLayout common_uniforms_bind_group_layout;
  WGPUBindGroup common_uniforms_bind_group;

  Transform mesh_transform;
  MeshUniforms mesh_uniforms;
  WGPUBuffer mesh_uniforms_buffer;
  WGPUBindGroupLayout mesh_uniforms_bind_group_layout;
  WGPUBindGroup mesh_uniforms_bind_group;
  GPUMesh mesh;

  WGPUTexture sand_texture;
  WGPUBindGroupLayout sand_texture_bind_group_layout;
  WGPUBindGroup sand_texture_bind_group;
} Renderer;

Renderer *renderer_new(SDL_Window *window, Assets *assets,
                       float current_time_secs);
void renderer_recreate_pipeline(Assets *assets, Renderer *renderer);
void renderer_destroy(Renderer *renderer);
void renderer_render(Renderer *renderer, float current_time_secs);

void renderer_initialize_for_window(Renderer *renderer, SDL_Window *window);

#endif // CUTTERENG_RENDERER_RENDERER_H
