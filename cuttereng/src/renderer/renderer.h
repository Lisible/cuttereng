#ifndef CUTTERENG_RENDERER_RENDERER_H
#define CUTTERENG_RENDERER_RENDERER_H

#include "../asset.h"
#include "../hash.h"
#include "../math/matrix.h"
#include "../memory.h"
#include "../transform.h"
#include "../vec.h"
#include "mesh.h"
#include "render_graph.h"
#include <SDL.h>
#include <webgpu/webgpu.h>
#define MAX_FILENAME_LENGTH 255
#define SHADER_DIR "shaders/"
#define MATERIAL_DIR "materials/"
#define TEXTURE_DIR "textures/"

DECL_HASH_TABLE(WGPURenderPipeline, HashTableRenderPipeline)
DECL_HASH_TABLE(WGPUShaderModule, HashTableShaderModule)
DECL_HASH_TABLE(WGPUTexture, HashTableTexture)

typedef struct GPUMaterial GPUMaterial;
DECL_HASH_TABLE(GPUMaterial *, HashTableMaterial)

typedef struct {
  mat4 projection_from_view;
  float current_time_secs;
  float _pad[15];
} CommonUniforms;

typedef struct {
  float world_from_local[16];
} MeshUniforms;

struct RendererContext {
  WGPURequiredLimits wgpu_limits;
  WGPUInstance wgpu_instance;
  WGPUAdapter wgpu_adapter;
  WGPUSurface wgpu_surface;
  WGPUDevice wgpu_device;
  WGPUTextureFormat wgpu_render_surface_texture_format;
  WGPUTextureFormat depth_texture_format;
};
typedef struct RendererContext RendererContext;

#define MAX_MESH_DRAW_PER_FRAME 1000

typedef struct {
  RenderGraphResourceHandle base_color;
  RenderGraphResourceHandle normal;
  RenderGraphResourceHandle position;
} GBuffer;

void GBuffer_init(GBuffer *g_buffer, WGPUDevice device,
                  RenderGraph *render_graph);
WGPUBindGroupLayout GBuffer_create_bind_group_layout(WGPUDevice device);
WGPUBindGroup
GBuffer_create_bind_group(GBuffer *gbuffer, RenderGraph *render_graph,
                          WGPUDevice device,
                          WGPUBindGroupLayout gbuffer_bind_group_layout);

struct RendererResources {
  WGPUTexture depth_texture;

  HashTableRenderPipeline *pipelines;
  HashTableShaderModule *shader_modules;
  HashTableTexture *textures;
  HashTableMaterial *materials;
  WGPUBindGroupLayout material_bind_group_layout;

  CommonUniforms common_uniforms;
  WGPUBuffer common_uniforms_buffer;
  WGPUBindGroupLayout common_uniforms_bind_group_layout;
  WGPUBindGroup common_uniforms_bind_group;

  GPUMesh triangle_mesh;
  MeshUniforms mesh_uniforms[MAX_MESH_DRAW_PER_FRAME];
  size_t mesh_uniform_count;
  WGPUBuffer mesh_uniforms_buffer;
  WGPUBindGroupLayout mesh_uniforms_bind_group_layout;
  WGPUBindGroup mesh_uniforms_bind_group;

  GBuffer g_buffer;
  WGPUBindGroupLayout g_buffer_bind_group_layout;
  WGPUBindGroup g_buffer_bind_group;
};
typedef struct RendererResources RendererResources;

typedef struct {
  Transform transform;
  char *material_identifier;
} DrawCommand;

DECL_VEC(DrawCommand, DrawCommandQueue)
typedef struct {
  Allocator *allocator;
  RendererContext ctx;
  RendererResources resources;
  DrawCommandQueue draw_commands;
} Renderer;

Renderer *renderer_new(Allocator *allocator, SDL_Window *window, Assets *assets,
                       float current_time_secs);
void renderer_destroy(Renderer *renderer);
void renderer_draw_mesh(Renderer *renderer, Transform *transform,
                        char *material_identifier);
void renderer_render(Allocator *frame_allocator, Renderer *renderer,
                     float current_time_secs);

void renderer_initialize_for_window(Renderer *renderer, SDL_Window *window);

#endif // CUTTERENG_RENDERER_RENDERER_H
