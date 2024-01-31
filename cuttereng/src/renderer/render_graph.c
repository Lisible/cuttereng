#include "render_graph.h"
#include "../assert.h"
#include "../bitset.h"
#include "../memory.h"
#include "../renderer/renderer.h"
#include <webgpu/webgpu.h>

void render_graph_init(RenderGraph *render_graph) {
  render_graph->pass_count = 0;
  render_graph->resource_count = 0;
}
void render_graph_deinit(RenderGraph *render_graph) {
  render_graph->pass_count = 0;

  for (size_t resource_index = 0; resource_index < render_graph->resource_count;
       resource_index++) {
    render_graph_resource_deinit(&render_graph->resources[resource_index]);
  }
  render_graph->resource_count = 0;
}
RenderGraphResourceHandle render_graph_register_texture_view(
    RenderGraph *render_graph, WGPUDevice device, WGPUTextureView texture_view,
    RenderGraphResourceUsage usage, WGPUTextureFormat texture_format) {
  RenderGraphResourceId resource_id = render_graph->resource_count;
  ASSERT(resource_id < RENDER_GRAPH_MAX_RESOURCES);

  WGPUSampler texture_sampler = wgpuDeviceCreateSampler(
      device, &(const WGPUSamplerDescriptor){
                  .addressModeU = WGPUAddressMode_ClampToEdge,
                  .addressModeV = WGPUAddressMode_ClampToEdge,
                  .addressModeW = WGPUAddressMode_ClampToEdge,
                  .lodMinClamp = 0.0f,
                  .lodMaxClamp = 1.0f,
                  .maxAnisotropy = 1});

  render_graph->resources[resource_id] = (RenderGraphResource){
      .type = RenderGraphResourceType_ImportedTexture,
      .usage = usage,
      .imported_texture = (RenderGraphImportedTextureResource){
          .texture_view = texture_view,
          .texture_sampler = texture_sampler,
          .texture_format = texture_format}};
  render_graph->resource_count++;
  return (RenderGraphResourceHandle){.id = resource_id, .version = 0};
}

RenderGraphResourceHandle
render_graph_create_texture(RenderGraph *render_graph,
                            RenderGraphResourceUsage usage, WGPUDevice device,
                            WGPUTextureFormat format) {
  RenderGraphResourceId resource_id = render_graph->resource_count;
  ASSERT(resource_id < RENDER_GRAPH_MAX_RESOURCES);

  WGPUTextureFormat *view_format = &format;
  if (format == WGPUTextureFormat_Depth24Plus) {
    view_format = NULL;
  }
  WGPUTexture texture = wgpuDeviceCreateTexture(
      device, &(const WGPUTextureDescriptor){
                  .dimension = WGPUTextureDimension_2D,
                  .format = format,
                  .viewFormatCount = view_format == NULL ? 0 : 1,
                  .viewFormats = view_format,
                  .mipLevelCount = 1,
                  .sampleCount = 1,
                  .size = {800, 600, 1},
                  .usage = WGPUTextureUsage_RenderAttachment |
                           WGPUTextureUsage_TextureBinding});
  if (!texture) {
    PANIC("Couldn't create texture");
  }

  WGPUTextureView texture_view = wgpuTextureCreateView(
      texture, &(const WGPUTextureViewDescriptor){
                   .aspect = format == WGPUTextureFormat_Depth24Plus
                                 ? WGPUTextureAspect_DepthOnly
                                 : WGPUTextureAspect_All,
                   .dimension = WGPUTextureViewDimension_2D,
                   .format = format,
                   .arrayLayerCount = 1,
                   .baseArrayLayer = 0,
                   .mipLevelCount = 1,
                   .baseMipLevel = 0});
  if (!texture_view) {
    PANIC("Couldn't create texture view");
  }

  WGPUSampler texture_sampler = wgpuDeviceCreateSampler(
      device, &(const WGPUSamplerDescriptor){
                  .addressModeU = WGPUAddressMode_ClampToEdge,
                  .addressModeV = WGPUAddressMode_ClampToEdge,
                  .addressModeW = WGPUAddressMode_ClampToEdge,
                  .lodMinClamp = 0.0f,
                  .lodMaxClamp = 1.0f,
                  .maxAnisotropy = 1});

  render_graph->resources[resource_id] =
      (RenderGraphResource){.type = RenderGraphResourceType_OwnedTexture,
                            .usage = usage,
                            .owned_texture = (RenderGraphOwnedTextureResource){
                                .texture = texture,
                                .texture_view = texture_view,
                                .texture_sampler = texture_sampler,
                                .texture_format = format}};
  render_graph->resource_count++;
  return (RenderGraphResourceHandle){.id = resource_id, .version = 0};
}

char *render_pass_descriptor_generate_identifier(
    Allocator *allocator, const RenderPassDescriptor *render_pass_descriptor) {
  ASSERT(render_pass_descriptor != NULL);
  static const size_t BOOLEAN_IDENTIFIER_LENGTH = 1;
  size_t identifier_length =
      strlen(render_pass_descriptor->shader_module_identifier);
  identifier_length += BOOLEAN_IDENTIFIER_LENGTH;
  identifier_length++; // Null terminator

  char *identifier =
      allocator_allocate_array(allocator, identifier_length, sizeof(char));
  identifier =
      strcat(identifier, render_pass_descriptor->shader_module_identifier);
  if (render_pass_descriptor->uses_vertex_buffer) {
    identifier = strcat(identifier, "t");
  } else {
    identifier = strcat(identifier, "f");
  }
  // allocator_allocate_array zero-initialize the allocated memory, so the
  // null terminator is already present

  return identifier;
}

void create_pipeline(RenderGraph *render_graph,
                     char *render_pass_pipeline_identifier,
                     RendererContext *ctx, RendererResources *res,
                     const RenderPassDescriptor *render_pass_descriptor) {
  ASSERT(render_pass_descriptor->bind_group_layout_count <=
         RENDER_PASS_MAX_BIND_GROUP_LAYOUTS);
  WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(
      ctx->wgpu_device,
      &(const WGPUPipelineLayoutDescriptor){
          .label = render_pass_descriptor->identifier,
          .bindGroupLayoutCount =
              render_pass_descriptor->bind_group_layout_count,
          .bindGroupLayouts = render_pass_descriptor->bind_group_layouts});

  WGPUShaderModule shader_module = HashTableShaderModule_get(
      res->shader_modules, render_pass_descriptor->shader_module_identifier);

  WGPUVertexAttribute vertex_attributes[] = {
      {.format = WGPUVertexFormat_Float32x3, .offset = 0, .shaderLocation = 0},
      {.format = WGPUVertexFormat_Float32x3,
       .offset = 3 * sizeof(float),
       .shaderLocation = 1},
      {.format = WGPUVertexFormat_Float32x2,
       .offset = 6 * sizeof(float),
       .shaderLocation = 2},
  };
  WGPUVertexBufferLayout vertex_buffer_layout = {
      .attributeCount = 3,
      .attributes = vertex_attributes,
      .arrayStride = sizeof(Vertex),
      .stepMode = WGPUVertexStepMode_Vertex};

  WGPUVertexState vertex_state = {.bufferCount = 1,
                                  .buffers = &vertex_buffer_layout,
                                  .constantCount = 0,
                                  .constants = NULL,
                                  .entryPoint = "vs_main",
                                  .module = shader_module};

  if (!render_pass_descriptor->uses_vertex_buffer) {
    vertex_state.bufferCount = 0;
    vertex_state.buffers = NULL;
  }

  WGPUDepthStencilState *depth_stencil_state = NULL;
  WGPUColorTargetState color_target_states[RENDER_PASS_MAX_COLOR_TARGET_STATES];
  size_t color_target_state_count = 0;
  for (size_t render_attachment_index = 0;
       render_attachment_index <
       render_pass_descriptor->render_attachment_count;
       render_attachment_index++) {
    const RenderPassRenderAttachment *render_attachment =
        &render_pass_descriptor->render_attachments[render_attachment_index];
    RenderGraphResource *render_attachment_resource =
        &render_graph
             ->resources[render_attachment->render_attachment_handle->id];
    ASSERT(render_attachment->render_attachment_handle != NULL);
    render_attachment->render_attachment_handle->version++;

    if (render_attachment->type ==
        RenderPassRenderAttachmentType_ColorAttachment) {
      WGPUColorTargetState *color_target_state =
          color_target_states + color_target_state_count;

      color_target_state->format =
          render_graph_resource_get_texture_format(render_attachment_resource);
      color_target_state->writeMask = WGPUColorWriteMask_All;
      color_target_state->blend = &(const WGPUBlendState){
          .color = (WGPUBlendComponent){.srcFactor = WGPUBlendFactor_SrcAlpha,
                                        .dstFactor =
                                            WGPUBlendFactor_OneMinusSrcAlpha,
                                        .operation = WGPUBlendOperation_Add},
          .alpha = (WGPUBlendComponent){.srcFactor = WGPUBlendFactor_One,
                                        .dstFactor = WGPUBlendFactor_Zero,
                                        .operation = WGPUBlendOperation_Add},
      };
      color_target_state->nextInChain = NULL;
      color_target_state_count++;
    } else {
      depth_stencil_state = &(WGPUDepthStencilState){
          .depthCompare = WGPUCompareFunction_Less,
          .depthWriteEnabled = true,
          .format = WGPUTextureFormat_Depth24Plus,
          .stencilWriteMask = 0,
          .stencilReadMask = 0,
          .stencilBack = {.compare = WGPUCompareFunction_Always,
                          .failOp = WGPUStencilOperation_Keep,
                          .depthFailOp = WGPUStencilOperation_Keep,
                          .passOp = WGPUStencilOperation_Keep},
          .stencilFront = {.compare = WGPUCompareFunction_Always,
                           .failOp = WGPUStencilOperation_Keep,
                           .depthFailOp = WGPUStencilOperation_Keep,
                           .passOp = WGPUStencilOperation_Keep}};
    }
  }

  WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(
      ctx->wgpu_device,
      &(const WGPURenderPipelineDescriptor){
          .label = render_pass_descriptor->identifier,
          .layout = pipeline_layout,
          .vertex = vertex_state,
          .fragment =
              &(WGPUFragmentState){.nextInChain = NULL,
                                   .constantCount = 0,
                                   .constants = NULL,
                                   .targets = color_target_states,
                                   .targetCount = color_target_state_count,
                                   .entryPoint = "fs_main",
                                   .module = shader_module},
          .depthStencil = depth_stencil_state,
          .multisample =
              (WGPUMultisampleState){.nextInChain = NULL,
                                     .count = 1,
                                     .mask = ~0u,
                                     .alphaToCoverageEnabled = false},
          .primitive = (WGPUPrimitiveState){
              .nextInChain = NULL,
              .topology = WGPUPrimitiveTopology_TriangleList,
              .cullMode = WGPUCullMode_Back,
              .frontFace = WGPUFrontFace_CCW,
              .stripIndexFormat = WGPUIndexFormat_Undefined

          }});
  HashTableRenderPipeline_set(res->pipelines, render_pass_pipeline_identifier,
                              pipeline);
  wgpuPipelineLayoutRelease(pipeline_layout);
}
RenderGraphPassId
render_graph_add_pass(Allocator *allocator, RenderGraph *render_graph,
                      RendererContext *ctx, RendererResources *res,
                      const RenderPassDescriptor *render_pass_descriptor) {
  ASSERT(ctx != NULL);
  ASSERT(res != NULL);
  ASSERT(render_graph != NULL);
  ASSERT(render_graph->pass_count < RENDER_GRAPH_MAX_PASS_COUNT);
  ASSERT(render_pass_descriptor != NULL);

  char *render_pass_pipeline_identifier = NULL;
  if (!render_pass_descriptor->has_no_render_pipeline) {
    render_pass_pipeline_identifier =
        render_pass_descriptor_generate_identifier(allocator,
                                                   render_pass_descriptor);
    if (!HashTableRenderPipeline_has(res->pipelines,
                                     render_pass_pipeline_identifier)) {
      create_pipeline(render_graph, render_pass_pipeline_identifier, ctx, res,
                      render_pass_descriptor);
    }
  }

  size_t pass_id = render_graph->pass_count;
  RenderPass *render_pass = &render_graph->passes[pass_id];
  render_pass->identifier = render_pass_descriptor->identifier;
  render_pass->pipeline_identifier = render_pass_pipeline_identifier;
  render_pass->dispatch_fn = render_pass_descriptor->dispatch_fn;
  render_pass->pass_data = render_pass_descriptor->pass_data;

  render_pass->write_count = render_pass_descriptor->render_attachment_count;
  ASSERT(render_pass->write_count <= RENDER_PASS_MAX_WRITE_RESOURCES);
  for (size_t i = 0; i < render_pass_descriptor->render_attachment_count; i++) {
    const RenderPassRenderAttachment *render_attachment =
        &render_pass_descriptor->render_attachments[i];
    RenderGraphResourceHandle *render_attachment_handle =
        render_attachment->render_attachment_handle;
    ASSERT(render_attachment_handle != NULL);
    render_pass->write[i] = (RenderGraphWriteResource){
        .id = render_attachment_handle->id,
        .type = RenderGraphWriteResourceType_Texture,
        .texture = (RenderGraphWriteTextureParameters){
            .load_op = render_attachment->load_op,
            .store_op = render_attachment->store_op}};
    render_attachment_handle->version++;
  }

  render_pass->read_count = render_pass_descriptor->read_resource_count;
  ASSERT(render_pass->read_count <= RENDER_PASS_MAX_READ_RESOURCES);
  for (size_t i = 0; i < render_pass_descriptor->read_resource_count; i++) {
    RenderGraphResourceHandle *resource_handle =
        render_pass_descriptor->read_resources[i];
    render_pass->read[i] = resource_handle->id;
    resource_handle->version++;
  }

  render_graph->pass_count++;
  LOG_TRACE("Added render pass %s", render_pass_descriptor->identifier);
  return pass_id;
}

void render_graph_compute_pass_ordering(RenderGraph *render_graph) {
  u64 pass_read_resources[RENDER_GRAPH_MAX_PASS_COUNT] = {0};
  ASSERT(BITNSLOTS(pass_read_resources[0]) <= RENDER_GRAPH_MAX_RESOURCES);
  size_t read_count[RENDER_GRAPH_MAX_PASS_COUNT] = {0};
  bool is_pass_ordered[RENDER_GRAPH_MAX_PASS_COUNT] = {false};
  size_t ordered_pass_count = 0;

  for (size_t pass_index = 0; pass_index < render_graph->pass_count;
       pass_index++) {
    read_count[pass_index] = render_graph->passes[pass_index].read_count;
    for (size_t read_index = 0; read_index < read_count[pass_index];
         read_index++) {
      BITSET(&pass_read_resources[pass_index],
             render_graph->passes[pass_index].read[read_index]);
    }
  }

  while (ordered_pass_count < render_graph->pass_count) {
    for (size_t pass_index = 0; pass_index < render_graph->pass_count;
         pass_index++) {
      RenderPass *pass = &render_graph->passes[pass_index];
      if (read_count[pass_index] == 0 && !is_pass_ordered[pass_index]) {
        render_graph->pass_ordering[ordered_pass_count] = pass_index;
        ordered_pass_count++;

        is_pass_ordered[pass_index] = true;
        for (size_t write_index = 0; write_index < pass->write_count;
             write_index++)
          for (size_t pi = 0; pi < render_graph->pass_count; pi++) {
            u64 *pass_read_bitset = &pass_read_resources[pi];
            RenderGraphResourceId write_resource_id =
                pass->write[write_index].id;
            if (BITTEST(pass_read_bitset, write_resource_id)) {
              BITCLEAR(pass_read_bitset, write_resource_id);
              read_count[pi]--;
            }
          }
      }
    }
  }
}

void render_graph_execute(RenderGraph *render_graph,
                          WGPUCommandEncoder command_encoder,
                          RendererResources *res,
                          DrawCommandQueue *draw_command_queue) {
  ASSERT(render_graph != NULL);
  ASSERT(command_encoder != NULL);
  ASSERT(res != NULL);
  ASSERT(draw_command_queue != NULL);
  LOG_DEBUG("Executing render graph");

  render_graph_compute_pass_ordering(render_graph);
  LOG_DEBUG("Pass ordering: ");
  for (size_t i = 0; i < render_graph->pass_count; i++) {
    LOG_DEBUG("- %s",
              render_graph->passes[render_graph->pass_ordering[i]].identifier);
  }

  for (size_t pass = 0; pass < render_graph->pass_count; pass++) {
    RenderPass *render_pass = &render_graph->passes[pass];
    WGPURenderPassColorAttachment
        color_attachments[RENDER_PASS_MAX_COLOR_TARGET_STATES];

    const WGPURenderPassDepthStencilAttachment *depth_stencil_attachment = NULL;
    size_t color_attachment_count = 0;
    for (unsigned write_index = 0; write_index < render_pass->write_count;
         write_index++) {
      RenderGraphWriteResource *write_resource =
          &render_pass->write[write_index];
      RenderGraphResource *resource =
          &render_graph->resources[write_resource->id];
      switch (resource->usage) {
      case RenderGraphResourceUsage_DepthStencilAttachment:
        depth_stencil_attachment =
            &(const WGPURenderPassDepthStencilAttachment){
                .view = render_graph_resource_get_texture_view(resource),
                .depthStoreOp = WGPUStoreOp_Store,
                .depthLoadOp = WGPULoadOp_Clear,
                .depthClearValue = 1.0,
                .stencilLoadOp = WGPULoadOp_Clear,
                .stencilStoreOp = WGPUStoreOp_Store,
                .stencilReadOnly = true};
        break;
      case RenderGraphResourceUsage_ColorAttachment:
        // FIXME will only work for direct binding
        color_attachments[color_attachment_count].view =
            render_graph_resource_get_texture_view(resource),
        color_attachments[color_attachment_count].loadOp =
            write_resource->texture.load_op;
        color_attachments[color_attachment_count].storeOp =
            write_resource->texture.store_op;
        color_attachments[color_attachment_count].clearValue =
            (WGPUColor){.r = 0.0, .g = 0.0, .b = 0.0, .a = 0.0};
        color_attachments[color_attachment_count].resolveTarget = NULL;
        color_attachment_count++;
        break;
      default:
        PANIC("Invalid resource usage");
      }
    }

    WGPURenderPassEncoder render_pass_encoder =
        wgpuCommandEncoderBeginRenderPass(
            command_encoder,
            &(const WGPURenderPassDescriptor){
                .label = render_pass->identifier,
                .colorAttachmentCount = color_attachment_count,
                .colorAttachments = color_attachments,
                .depthStencilAttachment = depth_stencil_attachment});
    if (render_pass->pipeline_identifier) {
      ASSERT(render_pass->dispatch_fn != NULL);
      WGPURenderPipeline pass_pipeline = HashTableRenderPipeline_get(
          res->pipelines, render_pass->pipeline_identifier);
      wgpuRenderPassEncoderSetPipeline(render_pass_encoder, pass_pipeline);
      uint32_t mesh_stride = 64;
      render_pass->dispatch_fn(render_pass_encoder, res, draw_command_queue,
                               &res->cube_mesh, mesh_stride,
                               render_pass->pass_data);
    }
    wgpuRenderPassEncoderEnd(render_pass_encoder);
    wgpuRenderPassEncoderRelease(render_pass_encoder);
  }
}

WGPUTextureView
render_graph_resource_get_texture_view(RenderGraphResource *resource) {
  switch (resource->type) {
  case RenderGraphResourceType_OwnedTexture:
    return resource->owned_texture.texture_view;
    break;
  case RenderGraphResourceType_ImportedTexture:
    return resource->imported_texture.texture_view;
    break;
  default:
    PANIC("Invalid render graph resource type");
  }
}
WGPUTextureFormat
render_graph_resource_get_texture_format(RenderGraphResource *resource) {
  if (resource->type == RenderGraphResourceType_OwnedTexture) {
    return resource->owned_texture.texture_format;
  } else if (resource->type == RenderGraphResourceType_ImportedTexture) {
    return resource->imported_texture.texture_format;
  } else {
    PANIC("invalid render graph resource type");
  }
}
void render_graph_resource_deinit(RenderGraphResource *resource) {
  switch (resource->type) {
  case RenderGraphResourceType_OwnedTexture:
    wgpuTextureViewRelease(resource->owned_texture.texture_view);
    wgpuSamplerRelease(resource->owned_texture.texture_sampler);
    wgpuTextureDestroy(resource->owned_texture.texture);
    wgpuTextureRelease(resource->owned_texture.texture);
    break;
  case RenderGraphResourceType_ImportedTexture:
    wgpuSamplerRelease(resource->imported_texture.texture_sampler);
    wgpuTextureViewRelease(resource->imported_texture.texture_view);
    break;
  default:
    PANIC("Unsupported resource type");
  }
}
