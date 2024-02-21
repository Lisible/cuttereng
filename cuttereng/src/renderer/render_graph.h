#ifndef CUTTERENG_RENDERER_RENDER_GRAPH_H
#define CUTTERENG_RENDERER_RENDER_GRAPH_H

#include "../memory.h"
#include "mesh.h"
#include <webgpu/webgpu.h>

#define RENDER_PASS_MAX_DEPTH_TARGET_STATES 1
#define RENDER_PASS_MAX_COLOR_TARGET_STATES 3
#define RENDER_PASS_MAX_RENDER_ATTACHMENTS                                     \
  (RENDER_PASS_MAX_COLOR_TARGET_STATES + RENDER_PASS_MAX_DEPTH_TARGET_STATES)
#define RENDER_GRAPH_MAX_PASS_COUNT 8
#define RENDER_PASS_MAX_BIND_GROUP_LAYOUTS 3
#define RENDER_GRAPH_MAX_TEXTURES 4
#define RENDER_GRAPH_MAX_TEXTURE_VIEWS 5
#define RENDER_PASS_MAX_READ_RESOURCES 5
#define RENDER_PASS_MAX_WRITE_RESOURCES 5
#define RENDER_GRAPH_MAX_RESOURCES 100

typedef size_t RenderGraphResourceId;
typedef struct {
  RenderGraphResourceId id;
  size_t version;
} RenderGraphResourceHandle;

typedef enum {
  RenderGraphBindGroup_Global = 0,
  RenderGraphBindGroup_Pass,
  RenderGraphBindGroup_Material,
  RenderGraphBindGroup_Object,
  RenderGraphBindGroup_Count
} RenderGraphBindGroup;

typedef size_t RenderGraphPassId;
typedef struct RendererContext RendererContext;
typedef struct RendererResources RendererResources;
typedef struct DrawCommandQueue DrawCommandQueue;

typedef struct {
  double r;
  double g;
  double b;
} ClearColor;

typedef struct {
  WGPUTextureView texture_view;
  ClearColor clear_color;
  bool clear;
} RenderPassColorAttachment;

typedef struct {
  WGPURenderPassEncoder pass_encoder;
  RendererResources *resources;
  DrawCommandQueue *command_queue;
  u32 mesh_stride;
  void *pass_data;
} RenderPassDispatchContext;

typedef void (*DispatchFn)(const RenderPassDispatchContext *);

typedef enum {
  RenderGraphWriteResourceType_Texture
} RenderGraphWriteResourceType;

typedef struct {
  WGPULoadOp load_op;
  WGPUStoreOp store_op;
} RenderGraphWriteTextureParameters;

typedef struct {
  RenderGraphResourceId id;
  RenderGraphWriteResourceType type;
  union {
    RenderGraphWriteTextureParameters texture;
  };
} RenderGraphWriteResource;

typedef struct {
  char *identifier;
  char *pipeline_identifier;
  RenderGraphResourceId read[RENDER_PASS_MAX_READ_RESOURCES];
  size_t read_count;
  RenderGraphWriteResource write[RENDER_PASS_MAX_WRITE_RESOURCES];
  size_t write_count;

  void *pass_data;
  DispatchFn dispatch_fn;
} RenderPass;

typedef enum {
  RenderGraphResourceType_Undefined = 0,
  RenderGraphResourceType_OwnedTexture,
  RenderGraphResourceType_ImportedTexture,
} RenderGraphResourceType;

typedef struct {
  WGPUTexture texture;
  WGPUTextureView texture_view;
  WGPUSampler texture_sampler;
  WGPUTextureFormat texture_format;
} RenderGraphOwnedTextureResource;

typedef struct {
  WGPUTextureView texture_view;
  WGPUSampler texture_sampler;
  WGPUTextureFormat texture_format;
} RenderGraphImportedTextureResource;

typedef struct {
  WGPUBindGroup bind_group;
  WGPUBindGroupLayout bind_group_layout;
} RenderGraphImportedBufferResource;

typedef struct {
  WGPUBindGroupLayout bind_group_layout;
} RenderGraphDynamicMaterialResource;

typedef enum {
  RenderGraphResourceUsage_Undefined = 0,
  RenderGraphResourceUsage_DepthStencilAttachment,
  RenderGraphResourceUsage_ColorAttachment,
  RenderGraphResourceUsage_UniformBuffer,
} RenderGraphResourceUsage;

typedef struct {
  RenderGraphResourceType type;
  RenderGraphResourceUsage usage;
  union {
    RenderGraphOwnedTextureResource owned_texture;
    RenderGraphImportedTextureResource imported_texture;
    RenderGraphImportedBufferResource imported_buffer;
    RenderGraphDynamicMaterialResource dynamic_material;
  };
} RenderGraphResource;
WGPUTextureView
render_graph_resource_get_texture_view(RenderGraphResource *resource);
WGPUTextureFormat
render_graph_resource_get_texture_format(RenderGraphResource *resource);
void render_graph_resource_deinit(RenderGraphResource *resource);

typedef struct {
  WGPUBindGroupLayoutDescriptor bind_group_layout[RenderGraphBindGroup_Count];
  RenderGraphResource resources[RENDER_GRAPH_MAX_RESOURCES];
  size_t resource_count;
  RenderPass passes[RENDER_GRAPH_MAX_PASS_COUNT];
  size_t pass_ordering[RENDER_GRAPH_MAX_PASS_COUNT];
  size_t pass_count;
} RenderGraph;

typedef enum {
  RenderPassRenderAttachmentType_ColorAttachment,
  RenderPassRenderAttachmentType_DepthAttachment
} RenderPassRenderAttachmentType;

typedef struct {
  RenderGraphResourceHandle *render_attachment_handle;
  RenderPassRenderAttachmentType type;
  WGPULoadOp load_op;
  WGPUStoreOp store_op;
} RenderPassRenderAttachment;

typedef struct {
  char *module_identifier;
  bool has_no_fragment_shader;
} RenderPassShaderModuleDescriptor;

typedef enum { CullMode_Back = 0, CullMode_Front, CullMode_None } CullMode;

typedef struct {
  char *identifier;
  RenderPassShaderModuleDescriptor shader_module;
  WGPUBindGroupLayout bind_group_layouts[RENDER_PASS_MAX_BIND_GROUP_LAYOUTS];
  size_t bind_group_layout_count;
  RenderPassRenderAttachment
      render_attachments[RENDER_PASS_MAX_RENDER_ATTACHMENTS];
  size_t render_attachment_count;
  RenderGraphResourceHandle *read_resources[RENDER_PASS_MAX_READ_RESOURCES];
  size_t read_resource_count;
  RenderGraphResourceHandle *write_resources[RENDER_PASS_MAX_WRITE_RESOURCES];
  size_t write_resource_count;

  DispatchFn dispatch_fn;
  void *pass_data;
  bool has_no_render_pipeline;
  bool uses_vertex_buffer;
  CullMode cull_mode;
} RenderPassDescriptor;

char *render_pass_descriptor_generate_identifier(
    Allocator *allocator, const RenderPassDescriptor *render_pass_descriptor);

void render_graph_init(RenderGraph *render_graph);
void render_graph_deinit(RenderGraph *render_graph);
RenderGraphResourceHandle render_graph_register_texture_view(
    RenderGraph *render_graph, WGPUDevice device, WGPUTextureView texture_view,
    RenderGraphResourceUsage usage, WGPUTextureFormat texture_format);
RenderGraphResourceHandle
render_graph_create_texture(RenderGraph *render_graph,
                            RenderGraphResourceUsage usage, WGPUDevice device,
                            WGPUTextureFormat format, u32 width, u32 height);
RenderGraphPassId
render_graph_add_pass(Allocator *allocator, RenderGraph *render_graph,
                      RendererContext *ctx, RendererResources *res,
                      const RenderPassDescriptor *render_pass_descriptor);
void render_graph_execute(RenderGraph *render_graph,
                          WGPUCommandEncoder command_encoder,
                          RendererResources *res,
                          DrawCommandQueue *draw_command_queue);

#endif // CUTTERENG_RENDERER_RENDER_GRAPH_H
