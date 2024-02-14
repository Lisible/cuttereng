#include "material.h"
#include "src/renderer/renderer.h"

typedef struct {
  WGPUBindGroup directional_light_depth_map;
} DirectionalLightShadowMapToGBufferPassData;
typedef struct {
  WGPUBindGroup g_buffer_bind_group;
} DeferredLightingPassData;
typedef struct {
  WGPUBindGroup deferred_lighting_result_bind_group;
} CompositionPassData;
void skybox_pass_dispatch(const RenderPassDispatchContext *ctx);
void mesh_pass_dispatch(const RenderPassDispatchContext *ctx);
void directional_light_space_depth_map_pass_dispatch(
    const RenderPassDispatchContext *ctx);
void directional_light_shadow_map_to_gbuffer_pass_dispatch(
    const RenderPassDispatchContext *ctx);
void deferred_lighting_pass_dispatch(const RenderPassDispatchContext *ctx);
void composition_pass_dispatch(const RenderPassDispatchContext *ctx);
uint32_t ceilToNextMultiple(uint32_t value, uint32_t step) {
  uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
  return step * divide_and_ceil;
}

typedef struct {
  WGPUBindGroupLayout deferred_lighting_result_bind_group_layout;
  WGPUBindGroup deferred_lighting_result_bind_group;
  WGPUBindGroupLayout g_buffer_bind_group_layout;
  WGPUBindGroup g_buffer_bind_group;
  WGPUBindGroupLayout directional_light_space_depth_map_bind_group_layout;
  WGPUBindGroup directional_light_space_depth_map_bind_group;
  DirectionalLightShadowMapToGBufferPassData
      directional_light_shadow_map_to_gbuffer_pass_data;
  DeferredLightingPassData deferred_lighting_pass_data;
  CompositionPassData composition_pass_data;
} DefaultPipelineState;

void renderer_default_pipeline(RendererContext *ctx, RendererResources *res,
                               void *render_pipeline_state,
                               DrawCommandQueue *draw_commands,
                               Allocator *frame_allocator,
                               RenderGraph *render_graph, WGPUQueue wgpu_queue,
                               WGPUTextureView surface_texture_view,
                               float current_time_secs) {
  DefaultPipelineState *state = render_pipeline_state;
  RenderGraphResourceHandle surface_texture_handle =
      render_graph_register_texture_view(
          render_graph, ctx->wgpu_device, surface_texture_view,
          RenderGraphResourceUsage_ColorAttachment,
          WGPUTextureFormat_BGRA8UnormSrgb);

  u32 internal_width = ctx->resolution_factor * ctx->surface_size.width;
  u32 internal_height = ctx->resolution_factor * ctx->surface_size.height;
  RenderGraphResourceHandle depth_texture = render_graph_create_texture(
      render_graph, RenderGraphResourceUsage_DepthStencilAttachment,
      ctx->wgpu_device, WGPUTextureFormat_Depth32Float, internal_width,
      internal_height);

  GBuffer g_buffer;
  GBuffer_init(&g_buffer, ctx->wgpu_device, render_graph, internal_width,
               internal_height);
  state->g_buffer_bind_group_layout =
      GBuffer_create_bind_group_layout(ctx->wgpu_device);
  state->g_buffer_bind_group =
      GBuffer_create_bind_group(&g_buffer, render_graph, ctx->wgpu_device,
                                state->g_buffer_bind_group_layout);

  render_graph_add_pass(
      frame_allocator, render_graph, ctx, res,
      &(const RenderPassDescriptor){
          .identifier = "g_buffer_pass",
          .bind_group_layouts = {res->common_uniforms_bind_group_layout,
                                 res->mesh_uniforms_bind_group_layout,
                                 res->material_bind_group_layout},
          .bind_group_layout_count = 3,
          .render_attachments =
              {(RenderPassRenderAttachment){
                   .type = RenderPassRenderAttachmentType_ColorAttachment,
                   .render_attachment_handle = &g_buffer.base_color,
                   .load_op = WGPULoadOp_Clear,
                   .store_op = WGPUStoreOp_Store},
               (RenderPassRenderAttachment){
                   .type = RenderPassRenderAttachmentType_ColorAttachment,
                   .render_attachment_handle = &g_buffer.normal,
                   .load_op = WGPULoadOp_Clear,
                   .store_op = WGPUStoreOp_Store},
               (RenderPassRenderAttachment){
                   .type = RenderPassRenderAttachmentType_ColorAttachment,
                   .render_attachment_handle = &g_buffer.position,
                   .load_op = WGPULoadOp_Clear,
                   .store_op = WGPUStoreOp_Store},
               (RenderPassRenderAttachment){
                   .type = RenderPassRenderAttachmentType_DepthAttachment,
                   .render_attachment_handle = &depth_texture,
                   .load_op = WGPULoadOp_Clear,
                   .store_op = WGPUStoreOp_Store}},
          .render_attachment_count = 4,
          .uses_vertex_buffer = true,
          .shader_module = {.module_identifier = "shader.wgsl"},
          .dispatch_fn = mesh_pass_dispatch});

  VEC_FOR_EACH(draw_commands, command, DrawCommand, {
    if (command->material.type != MaterialType_Shader) {
      continue;
    }

    // FIXME this will create a new pass for each single mesh with a custom
    // shader if several meshes use a shader, there should only be one pass for
    // all of these
    render_graph_add_pass(
        frame_allocator, render_graph, ctx, res,
        &(const RenderPassDescriptor){
            .identifier = command->material.material_shader,
            .bind_group_layouts = {res->common_uniforms_bind_group_layout,
                                   res->mesh_uniforms_bind_group_layout,
                                   res->material_bind_group_layout},
            .bind_group_layout_count = 3,
            .render_attachments =
                {(RenderPassRenderAttachment){
                     .type = RenderPassRenderAttachmentType_ColorAttachment,
                     .render_attachment_handle = &g_buffer.base_color,
                     .load_op = WGPULoadOp_Clear,
                     .store_op = WGPUStoreOp_Store},
                 (RenderPassRenderAttachment){
                     .type = RenderPassRenderAttachmentType_ColorAttachment,
                     .render_attachment_handle = &g_buffer.normal,
                     .load_op = WGPULoadOp_Clear,
                     .store_op = WGPUStoreOp_Store},
                 (RenderPassRenderAttachment){
                     .type = RenderPassRenderAttachmentType_ColorAttachment,
                     .render_attachment_handle = &g_buffer.position,
                     .load_op = WGPULoadOp_Clear,
                     .store_op = WGPUStoreOp_Store},
                 (RenderPassRenderAttachment){
                     .type = RenderPassRenderAttachmentType_DepthAttachment,
                     .render_attachment_handle = &depth_texture,
                     .load_op = WGPULoadOp_Clear,
                     .store_op = WGPUStoreOp_Store}},
            .render_attachment_count = 4,
            .uses_vertex_buffer = true,
            .shader_module = {.module_identifier =
                                  command->material.material_shader},
            .dispatch_fn = mesh_pass_dispatch});
  });

  u32 depth_map_resolution = 1024;
  RenderGraphResourceHandle directional_light_space_depth_map =
      render_graph_create_texture(
          render_graph, RenderGraphResourceUsage_DepthStencilAttachment,
          ctx->wgpu_device, WGPUTextureFormat_Depth32Float,
          depth_map_resolution, depth_map_resolution);

  state->directional_light_space_depth_map_bind_group_layout =
      wgpuDeviceCreateBindGroupLayout(
          ctx->wgpu_device,
          &(const WGPUBindGroupLayoutDescriptor){
              .label = "directional_light_space_depth_map_bind_group_layout",
              .entryCount = 2,
              .entries = (const WGPUBindGroupLayoutEntry[]){
                  (WGPUBindGroupLayoutEntry){
                      .binding = 0,
                      .visibility = WGPUShaderStage_Fragment,
                      .texture =
                          (WGPUTextureBindingLayout){
                              .sampleType =
                                  WGPUTextureSampleType_UnfilterableFloat,
                              .viewDimension = WGPUTextureViewDimension_2D}},
                  (WGPUBindGroupLayoutEntry){
                      .binding = 1,
                      .visibility = WGPUShaderStage_Fragment,
                      .sampler =
                          (WGPUSamplerBindingLayout){
                              .type = WGPUSamplerBindingType_NonFiltering}},
              }});

  RenderGraphOwnedTextureResource
      directional_light_space_depth_map_texture_resource =
          render_graph->resources[directional_light_space_depth_map.id]
              .owned_texture;
  state->directional_light_space_depth_map_bind_group =
      wgpuDeviceCreateBindGroup(
          ctx->wgpu_device,
          &(const WGPUBindGroupDescriptor){
              .label = "directional_light_space_depth_map_bind_group",
              .layout =
                  state->directional_light_space_depth_map_bind_group_layout,
              .entryCount = 2,
              .entries = (const WGPUBindGroupEntry[]){
                  (WGPUBindGroupEntry){
                      .binding = 0,
                      .textureView =
                          directional_light_space_depth_map_texture_resource
                              .texture_view},
                  (WGPUBindGroupEntry){
                      .binding = 1,
                      .sampler =
                          directional_light_space_depth_map_texture_resource
                              .texture_sampler},
              }});

  render_graph_add_pass(
      frame_allocator, render_graph, ctx, res,
      &(const RenderPassDescriptor){
          .identifier = "directional_light_space_depth_map_pass",
          .bind_group_layouts = {res->common_uniforms_bind_group_layout,
                                 res->mesh_uniforms_bind_group_layout},
          .bind_group_layout_count = 2,
          .render_attachments = {(RenderPassRenderAttachment){
              .type = RenderPassRenderAttachmentType_DepthAttachment,
              .render_attachment_handle = &directional_light_space_depth_map}},
          .render_attachment_count = 1,
          .uses_vertex_buffer = true,
          .cull_mode = CullMode_Front,
          .shader_module = {.module_identifier =
                                "directional_light_space_depth_map.wgsl",
                            .has_no_fragment_shader = true},
          .dispatch_fn = directional_light_space_depth_map_pass_dispatch});

  state->directional_light_shadow_map_to_gbuffer_pass_data =
      (DirectionalLightShadowMapToGBufferPassData){
          .directional_light_depth_map =
              state->directional_light_space_depth_map_bind_group};
  render_graph_add_pass(
      frame_allocator, render_graph, ctx, res,
      &(const RenderPassDescriptor){
          .identifier = "directional_light_shadow_map_to_gbuffer",
          .bind_group_layouts =
              {state->directional_light_space_depth_map_bind_group_layout},
          .bind_group_layout_count = 1,
          .render_attachments = {(RenderPassRenderAttachment){
              .type = RenderPassRenderAttachmentType_ColorAttachment,
              .load_op = WGPULoadOp_Clear,
              .store_op = WGPUStoreOp_Store,
              .render_attachment_handle =
                  &g_buffer.directional_light_space_depth_map}},
          .render_attachment_count = 1,
          .uses_vertex_buffer = false,
          .shader_module =
              {
                  .module_identifier =
                      "directional_light_shadow_map_to_gbuffer.wgsl",
              },
          .pass_data =
              &state->directional_light_shadow_map_to_gbuffer_pass_data,
          .dispatch_fn =
              directional_light_shadow_map_to_gbuffer_pass_dispatch});

  RenderGraphResourceHandle deferred_lighting_result =
      render_graph_create_texture(
          render_graph, RenderGraphResourceUsage_ColorAttachment,
          ctx->wgpu_device, WGPUTextureFormat_RGBA8UnormSrgb,
          ctx->surface_size.width, ctx->surface_size.height);
  state->deferred_lighting_pass_data = (DeferredLightingPassData){
      .g_buffer_bind_group = state->g_buffer_bind_group};
  render_graph_add_pass(
      frame_allocator, render_graph, ctx, res,
      &(const RenderPassDescriptor){
          .identifier = "deferred_lighting_pass",
          .bind_group_layouts = {res->common_uniforms_bind_group_layout,
                                 state->g_buffer_bind_group_layout},
          .bind_group_layout_count = 2,
          .render_attachments = {(RenderPassRenderAttachment){
              .type = RenderPassRenderAttachmentType_ColorAttachment,
              .render_attachment_handle = &deferred_lighting_result,
              .load_op = WGPULoadOp_Load,
              .store_op = WGPUStoreOp_Store}},
          .render_attachment_count = 1,
          .read_resources = {&g_buffer.base_color, &g_buffer.normal,
                             &g_buffer.position,
                             &g_buffer.directional_light_space_depth_map,
                             &surface_texture_handle},
          .read_resource_count = 5,
          .shader_module = {.module_identifier = "deferred_lighting.wgsl"},
          .pass_data = &state->deferred_lighting_pass_data,
          .dispatch_fn = deferred_lighting_pass_dispatch});

  render_graph_add_pass(
      frame_allocator, render_graph, ctx, res,
      &(const RenderPassDescriptor){
          .identifier = "skybox_pass",
          .bind_group_layout_count = 0,
          .render_attachments = {(RenderPassRenderAttachment){
              .type = RenderPassRenderAttachmentType_ColorAttachment,
              .render_attachment_handle = &surface_texture_handle,
              .load_op = WGPULoadOp_Clear,
              .store_op = WGPUStoreOp_Store}},
          .render_attachment_count = 1,
          .read_resource_count = 0,
          .uses_vertex_buffer = false,
          .shader_module = {.module_identifier = "skybox.wgsl"},
          .dispatch_fn = skybox_pass_dispatch});

  state->deferred_lighting_result_bind_group_layout =
      wgpuDeviceCreateBindGroupLayout(
          ctx->wgpu_device,
          &(const WGPUBindGroupLayoutDescriptor){
              .entryCount = 2,
              .entries = (const WGPUBindGroupLayoutEntry[]){
                  (WGPUBindGroupLayoutEntry){
                      .binding = 0,
                      .visibility = WGPUShaderStage_Fragment,
                      .texture =
                          (WGPUTextureBindingLayout){
                              .sampleType = WGPUTextureSampleType_Float,
                              .viewDimension = WGPUTextureViewDimension_2D}},
                  (WGPUBindGroupLayoutEntry){
                      .binding = 1,
                      .visibility = WGPUShaderStage_Fragment,
                      .sampler =
                          (WGPUSamplerBindingLayout){
                              .type = WGPUSamplerBindingType_Filtering},
                  }}});

  state->deferred_lighting_result_bind_group = wgpuDeviceCreateBindGroup(
      ctx->wgpu_device,
      &(const WGPUBindGroupDescriptor){
          .entryCount = 2,
          .entries =
              (const WGPUBindGroupEntry[]){
                  (WGPUBindGroupEntry){
                      .binding = 0,
                      .textureView =
                          render_graph->resources[deferred_lighting_result.id]
                              .owned_texture.texture_view},
                  (WGPUBindGroupEntry){
                      .binding = 1,
                      .sampler =
                          render_graph->resources[deferred_lighting_result.id]
                              .owned_texture.texture_sampler}},
          .layout = state->deferred_lighting_result_bind_group_layout});

  state->composition_pass_data =
      (CompositionPassData){.deferred_lighting_result_bind_group =
                                state->deferred_lighting_result_bind_group};
  render_graph_add_pass(
      frame_allocator, render_graph, ctx, res,
      &(const RenderPassDescriptor){
          .identifier = "composition_pass",
          .bind_group_layouts =
              {state->deferred_lighting_result_bind_group_layout},
          .bind_group_layout_count = 1,
          .render_attachments = {(RenderPassRenderAttachment){
              .type = RenderPassRenderAttachmentType_ColorAttachment,
              .render_attachment_handle = &surface_texture_handle,
              .load_op = WGPULoadOp_Load,
              .store_op = WGPUStoreOp_Store}},
          .render_attachment_count = 1,
          .read_resources = {&deferred_lighting_result,
                             &surface_texture_handle},
          .read_resource_count = 2,
          .shader_module = {.module_identifier = "composition.wgsl"},
          .pass_data = &state->composition_pass_data,
          .dispatch_fn = composition_pass_dispatch});

  for (size_t light_index = 0; light_index < res->light_count; light_index++) {
    Light *light = &res->lights[light_index];
    if (light->type == LightType_Directional) {
      res->common_uniforms.directional_light = light->directional_light;

      // computing the light space view proj matrix
      mat4 light_space_projection;
      mat4_set_to_orthographic(light_space_projection, 1.0, 200.0, -200.0,
                               200.0, 200.0, -200.0);

      v3f target = {0};
      mat4 light_space_view;
      mat4_look_at(light_space_view, &light->directional_light.position,
                   &target, &(const v3f){.y = 1.0});
      mat4_transpose(light_space_view);

      mat4 light_space_projection_from_view_from_local = MAT4_IDENTITY;
      mat4_mul(light_space_projection, light_space_view,
               light_space_projection_from_view_from_local);
      mat4_transpose(light_space_projection_from_view_from_local);
      memcpy(res->common_uniforms.light_space_projection_from_view_from_local,
             light_space_projection_from_view_from_local,
             16 * sizeof(mat4_value_type));
    }
  }
  res->common_uniforms.current_time_secs = current_time_secs;
  wgpuQueueWriteBuffer(wgpu_queue, res->common_uniforms_buffer, 0,
                       &res->common_uniforms, sizeof(CommonUniforms));

  size_t mesh_uniform_stride = ceilToNextMultiple(
      sizeof(MeshUniforms),
      ctx->wgpu_limits.limits.minUniformBufferOffsetAlignment);
  for (size_t command_index = 0; command_index < draw_commands->length;
       command_index++) {
    DrawCommand *command = &draw_commands->data[command_index];
    MeshUniforms mesh_uniforms = {0};
    transform_matrix(&command->transform,
                     (float *)mesh_uniforms.world_from_local);
    mat4_transpose(mesh_uniforms.world_from_local);
    wgpuQueueWriteBuffer(wgpu_queue, res->mesh_uniforms_buffer,
                         command_index * mesh_uniform_stride, &mesh_uniforms,
                         sizeof(MeshUniforms));
  }
}

void renderer_default_pipeline_cleanup(void *s) {
  DefaultPipelineState *state = (DefaultPipelineState *)s;
  wgpuBindGroupLayoutRelease(state->deferred_lighting_result_bind_group_layout);
  wgpuBindGroupRelease(state->deferred_lighting_result_bind_group);
  wgpuBindGroupLayoutRelease(state->g_buffer_bind_group_layout);
  wgpuBindGroupRelease(state->g_buffer_bind_group);
  wgpuBindGroupLayoutRelease(
      state->directional_light_space_depth_map_bind_group_layout);
  wgpuBindGroupRelease(state->directional_light_space_depth_map_bind_group);
}

void skybox_pass_dispatch(const RenderPassDispatchContext *ctx) {
  wgpuRenderPassEncoderDraw(ctx->pass_encoder, 3, 1, 0, 0);
}

void mesh_pass_dispatch(const RenderPassDispatchContext *ctx) {
  WGPURenderPassEncoder render_pass_encoder = ctx->pass_encoder;
  RendererResources *res = ctx->resources;
  DrawCommandQueue *command_queue = ctx->command_queue;
  u32 mesh_stride = ctx->mesh_stride;
  wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 0,
                                    res->common_uniforms_bind_group, 0, NULL);
  for (size_t command_index = 0;
       command_index < DrawCommandQueue_length(command_queue);
       command_index++) {
    DrawCommand *command = &command_queue->data[command_index];

    GPUMesh *mesh = HashTableGPUMesh_get(res->meshes, command->mesh_identifier);
    gpu_mesh_bind(render_pass_encoder, mesh);
    u32 stride = mesh_stride * command_index;
    wgpuRenderPassEncoderSetBindGroup(
        render_pass_encoder, 1, res->mesh_uniforms_bind_group, 1, &stride);

    GPUMaterial *material;
    if (command->material.type == MaterialType_Basic) {
      material = HashTableMaterial_get(res->materials,
                                       command->material.material_identifier);
    } else {
      material = HashTableMaterial_get(res->materials, "water.json");
    }
    wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 2,
                                      material->bind_group, 0, 0);
    wgpuRenderPassEncoderDraw(render_pass_encoder, mesh->vertex_count, 1, 0, 0);
  }
}

void directional_light_space_depth_map_pass_dispatch(
    const RenderPassDispatchContext *ctx) {
  WGPURenderPassEncoder render_pass_encoder = ctx->pass_encoder;
  RendererResources *res = ctx->resources;
  DrawCommandQueue *command_queue = ctx->command_queue;
  u32 mesh_stride = ctx->mesh_stride;
  wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 0,
                                    res->common_uniforms_bind_group, 0, NULL);
  for (size_t command_index = 0;
       command_index < DrawCommandQueue_length(command_queue);
       command_index++) {
    DrawCommand *command = &command_queue->data[command_index];
    GPUMesh *mesh = HashTableGPUMesh_get(res->meshes, command->mesh_identifier);
    gpu_mesh_bind(render_pass_encoder, mesh);
    u32 stride = mesh_stride * command_index;
    wgpuRenderPassEncoderSetBindGroup(
        render_pass_encoder, 1, res->mesh_uniforms_bind_group, 1, &stride);
    wgpuRenderPassEncoderDraw(render_pass_encoder, mesh->vertex_count, 1, 0, 0);
  }
}

void directional_light_shadow_map_to_gbuffer_pass_dispatch(
    const RenderPassDispatchContext *ctx) {
  DirectionalLightShadowMapToGBufferPassData *data = ctx->pass_data;
  wgpuRenderPassEncoderSetBindGroup(ctx->pass_encoder, 0,
                                    data->directional_light_depth_map, 0, NULL);
  wgpuRenderPassEncoderDraw(ctx->pass_encoder, 6, 1, 0, 0);
}

void deferred_lighting_pass_dispatch(const RenderPassDispatchContext *ctx) {
  DeferredLightingPassData *pass_data = ctx->pass_data;
  WGPURenderPassEncoder render_pass_encoder = ctx->pass_encoder;
  RendererResources *res = ctx->resources;
  wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 0,
                                    res->common_uniforms_bind_group, 0, NULL);
  wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 1,
                                    pass_data->g_buffer_bind_group, 0, NULL);
  wgpuRenderPassEncoderDraw(render_pass_encoder, 6, 1, 0, 0);
}

void composition_pass_dispatch(const RenderPassDispatchContext *ctx) {
  CompositionPassData *pass_data = ctx->pass_data;
  WGPURenderPassEncoder render_pass_encoder = ctx->pass_encoder;
  wgpuRenderPassEncoderSetBindGroup(
      render_pass_encoder, 0, pass_data->deferred_lighting_result_bind_group, 0,
      NULL);
  wgpuRenderPassEncoderDraw(render_pass_encoder, 6, 1, 0, 0);
}

RendererRenderPipeline *
renderer_default_render_pipeline_create(Allocator *allocator) {
  RendererRenderPipeline *pipeline =
      allocator_allocate(allocator, sizeof(RendererRenderPipeline));
  pipeline->pipeline_state =
      allocator_allocate(allocator, sizeof(DefaultPipelineState));
  pipeline->fn = renderer_default_pipeline;
  pipeline->cleanup_fn = renderer_default_pipeline_cleanup;
  return pipeline;
}
void renderer_default_render_pipeline_destroy(
    Allocator *allocator, RendererRenderPipeline *pipeline) {
  allocator_free(allocator, pipeline->pipeline_state);
  allocator_free(allocator, pipeline);
}
