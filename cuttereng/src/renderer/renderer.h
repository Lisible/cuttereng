#ifndef CUTTERENG_RENDERER_RENDERER_H
#define CUTTERENG_RENDERER_RENDERER_H

#include "../asset.h"
#include "../graphics/light.h"
#include "../hash.h"
#include "../math/matrix.h"
#include "../memory.h"
#include "../transform.h"
#include "../vec.h"
#include "material.h"
#include "render_graph.h"
#include "resource_cache.h"
#include <SDL.h>
#include <webgpu/webgpu.h>
#define MAX_FILENAME_LENGTH 255
#define MAX_LIGHT_COUNT 1
#define SHADER_DIR "shaders/"
#define MATERIAL_DIR "materials/"
#define FONT_DIR "fonts/"
#define TEXTURE_DIR "textures/"

DECL_HASH_TABLE(WGPURenderPipeline, HashTableRenderPipeline)
DECL_HASH_TABLE(WGPUShaderModule, HashTableShaderModule)
DECL_HASH_TABLE(WGPUTexture, HashTableTexture)

typedef struct GPUMaterial GPUMaterial;
DECL_HASH_TABLE(GPUMaterial *, HashTableMaterial)

typedef struct {
  mat4 projection_from_view;
  mat4 inverse_projection_from_view;
  mat4 light_space_projection_from_view_from_local;
  DirectionalLight directional_light;
  float _pad;
  v3f view_position;
  float current_time_secs;
} CommonUniforms;

typedef struct {
  float world_from_local[16];
} MeshUniforms;

typedef struct {
  u32 width;
  u32 height;
} SurfaceSize;

struct RendererContext {
  WGPURequiredLimits wgpu_limits;
  WGPUInstance wgpu_instance;
  WGPUAdapter wgpu_adapter;
  WGPUSurface wgpu_surface;
  WGPUDevice wgpu_device;
  WGPUTextureFormat wgpu_render_surface_texture_format;
  WGPUTextureFormat depth_texture_format;
  SurfaceSize surface_size;
  float resolution_factor;
};
typedef struct RendererContext RendererContext;

#define MAX_MESH_DRAW_PER_FRAME 2048

typedef struct {
  RenderGraphResourceHandle base_color;
  RenderGraphResourceHandle normal;
  RenderGraphResourceHandle position;
  RenderGraphResourceHandle directional_light_space_depth_map;
} GBuffer;

void GBuffer_init(GBuffer *g_buffer, WGPUDevice device,
                  RenderGraph *render_graph, u32 width, u32 height);
WGPUBindGroupLayout GBuffer_create_bind_group_layout(WGPUDevice device);
WGPUBindGroup
GBuffer_create_bind_group(GBuffer *gbuffer, RenderGraph *render_graph,
                          WGPUDevice device,
                          WGPUBindGroupLayout gbuffer_bind_group_layout);

struct RendererResources {
  WGPUTexture depth_texture;

  HashTableRenderPipeline *pipelines;
  ResourceCaches resource_caches;
  WGPUBindGroupLayout material_bind_group_layout;

  CommonUniforms common_uniforms;
  WGPUBuffer common_uniforms_buffer;
  WGPUBindGroupLayout common_uniforms_bind_group_layout;
  WGPUBindGroup common_uniforms_bind_group;

  MeshUniforms mesh_uniforms[MAX_MESH_DRAW_PER_FRAME];
  size_t mesh_uniform_count;
  WGPUBuffer mesh_uniforms_buffer;
  WGPUBindGroupLayout mesh_uniforms_bind_group_layout;
  WGPUBindGroup mesh_uniforms_bind_group;

  GBuffer g_buffer;
  WGPUBindGroupLayout g_buffer_bind_group_layout;
  WGPUBindGroup g_buffer_bind_group;

  Light lights[MAX_LIGHT_COUNT];
  size_t light_count;
};
typedef struct RendererResources RendererResources;

typedef struct {
  Transform transform;
  AssetHandle mesh_handle;
  AssetHandle material_handle;
  bool uses_shader_material;
} DrawCommand;

void DrawCommand_deinit(Allocator *allocator, DrawCommand *command);

DECL_VEC(DrawCommand, DrawCommandQueue)

typedef void (*RendererRenderPipelineFn)(
    RendererContext *ctx, RendererResources *res,
    void *render_pipeline_static_state, void *render_pipeline_state,
    DrawCommandQueue *draw_commands, Allocator *frame_allocator,
    RenderGraph *render_graph, WGPUQueue wgpu_queue,
    WGPUTextureView surface_texture_view, float current_time_secs);

typedef void (*RendererRenderPipelineCleanupFn)(void *pipeline_state);

typedef struct {
  void *pipeline_static_state;
  void *pipeline_state;
  RendererRenderPipelineFn fn;
  RendererRenderPipelineCleanupFn cleanup_fn;
} RendererRenderPipeline;

typedef struct {
  Allocator *allocator;
  RendererContext ctx;
  RendererResources resources;
  DrawCommandQueue draw_commands;
  RendererRenderPipeline *render_pipeline;
} Renderer;

Renderer *renderer_new(Allocator *allocator, SDL_Window *window,
                       Assets *assets);
void renderer_destroy(Renderer *renderer);
void renderer_add_light(Renderer *renderer, const Light *light);
void renderer_set_view_position(Renderer *renderer, v3f *view_position);
void renderer_set_view_projection(Renderer *renderer, mat4 view_projection);
void renderer_draw_mesh(Renderer *renderer, Transform *transform,
                        AssetHandle mesh_handle, AssetHandle material_handle);
void renderer_draw_mesh_with_shader_material(Renderer *renderer,
                                             Transform *transform,
                                             AssetHandle mesh_handle,
                                             AssetHandle material_handle);
void renderer_render(Allocator *frame_allocator, Renderer *renderer,
                     Assets *assets, float current_time_secs);
void renderer_clear_caches(Renderer *renderer);
void renderer_load_resources(Renderer *renderer, Assets *assets);

void renderer_initialize_for_window(Renderer *renderer, SDL_Window *window);

#endif // CUTTERENG_RENDERER_RENDERER_H
