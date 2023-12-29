#include "render_graph.h"
#include "../assert.h"
#include "../asset.h"
#include "shader.h"
#include "src/renderer/renderer.h"
#include <webgpu/webgpu.h>

void render_graph_init(RenderGraph *render_graph) {
  render_graph->pass_count = 0;
}

void render_graph_add_pass(RenderGraph *render_graph, Assets *assets,
                           RendererContext *ctx, RendererResources *res,
                           const RenderPassDescriptor *render_pass_descriptor) {
  ASSERT(render_graph != NULL);
  ASSERT(ctx != NULL);
  ASSERT(res != NULL);
  ASSERT(render_pass_descriptor != NULL);

  // TODO extract this into a function (generating a missing render_pipeline)
  if (!HashTableRenderPipeline_has(res->pipelines,
                                   render_pass_descriptor->identifier)) {

    WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(
        ctx->wgpu_device,
        &(const WGPUPipelineLayoutDescriptor){
            .label = render_pass_descriptor->identifier,
            .bindGroupLayoutCount = 2,
            .bindGroupLayouts =
                (WGPUBindGroupLayout[]){res->common_uniforms_bind_group_layout,
                                        res->mesh_uniforms_bind_group_layout},
        });

    WGPUShaderModule shader_module = HashTableShaderModule_get(
        res->shader_modules, render_pass_descriptor->shader_module_identifier);

    // TODO extract this into a function (generating a missing shader module)
    if (!shader_module) {
      Shader *shader = assets_fetch(
          assets, Shader, render_pass_descriptor->shader_module_identifier);
      WGPUShaderModuleWGSLDescriptor shader_module_wgsl_descriptor = {
          .chain =
              (WGPUChainedStruct){.sType =
                                      WGPUSType_ShaderModuleWGSLDescriptor},
          .code = shader->source};
      shader_module = wgpuDeviceCreateShaderModule(
          ctx->wgpu_device,
          &(const WGPUShaderModuleDescriptor){
              .label = render_pass_descriptor->shader_module_identifier,
              .nextInChain = &shader_module_wgsl_descriptor.chain});
      HashTableShaderModule_set(
          res->shader_modules, render_pass_descriptor->shader_module_identifier,
          shader_module);
    }

    WGPUVertexState vertex_state = {
        .bufferCount = 1,
        .buffers =
            &(const WGPUVertexBufferLayout){
                .attributeCount = 3,
                .attributes =
                    (const WGPUVertexAttribute[]){
                        (WGPUVertexAttribute){.format =
                                                  WGPUVertexFormat_Float32x3,
                                              .offset = 0,
                                              .shaderLocation = 0},
                        (WGPUVertexAttribute){.format =
                                                  WGPUVertexFormat_Float32x3,
                                              .offset = 3 * sizeof(float),
                                              .shaderLocation = 1},
                        (WGPUVertexAttribute){.format =
                                                  WGPUVertexFormat_Float32x2,
                                              .offset = 6 * sizeof(float),
                                              .shaderLocation = 2},
                    }},
        .entryPoint = "vs_main",
        .module = shader_module};

    if (!render_pass_descriptor->uses_vertex_buffer) {
      vertex_state.bufferCount = 0;
      vertex_state.buffers = NULL;
    }

    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(
        ctx->wgpu_device,
        &(const WGPURenderPipelineDescriptor){
            .label = render_pass_descriptor->identifier,
            .layout = pipeline_layout,
            .vertex = vertex_state,
            .fragment =
                &(WGPUFragmentState){
                    .nextInChain = NULL,
                    .constantCount = 0,
                    .constants = NULL,
                    .targets =
                        &(WGPUColorTargetState){
                            .nextInChain = NULL,
                            .format = ctx->wgpu_render_surface_texture_format,
                            .blend =
                                &(const WGPUBlendState){
                                    .color =
                                        (WGPUBlendComponent){
                                            .srcFactor =
                                                WGPUBlendFactor_SrcAlpha,
                                            .dstFactor =
                                                WGPUBlendFactor_OneMinusSrcAlpha,
                                            .operation =
                                                WGPUBlendOperation_Add},
                                    .alpha =
                                        (WGPUBlendComponent){
                                            .srcFactor = WGPUBlendFactor_Zero,
                                            .dstFactor = WGPUBlendFactor_One,
                                            .operation =
                                                WGPUBlendOperation_Add}},

                            .writeMask = WGPUColorWriteMask_All,
                        },
                    .targetCount = 1,
                    .entryPoint = "fs_main",
                    .module = shader_module},
            .depthStencil =
                &(const WGPUDepthStencilState){
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
                                     .passOp = WGPUStencilOperation_Keep}},
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
    HashTableRenderPipeline_set(res->pipelines,
                                render_pass_descriptor->identifier, pipeline);
  }

  ASSERT(render_graph->pass_count < RENDER_GRAPH_MAX_PASS_COUNT);
  render_graph->passes[render_graph->pass_count].pipeline_identifier =
      render_pass_descriptor->identifier;
  render_graph->pass_count++;
  LOG_TRACE("Added render pass %s", render_pass_descriptor->identifier);
}
