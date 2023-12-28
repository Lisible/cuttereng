#include "renderer.h"
#include "../assert.h"
#include "../image.h"
#include "../log.h"
#include "../math/matrix.h"
#include "../math/quaternion.h"
#include "../memory.h"
#include "../transform.h"
#include "shader.h"
#include "webgpu/webgpu.h"
#include <SDL2/SDL_syswm.h>
#include <math.h>

void on_queue_submitted_work_done(WGPUQueueWorkDoneStatus status,
                                  void *user_data) {
  LOG_TRACE("Queued work finished with status: %d", status);
}

void on_device_error(WGPUErrorType error_type, const char *message,
                     void *user_data) {
  LOG_ERROR("Uncaptured device error: type %d", error_type);
  if (message) {
    LOG_ERROR("(%s)", message);
  }
}

typedef struct {
  WGPUAdapter adapter;
  bool request_ended;
} AdapterRequestUserData;

void on_adapter_request_ended(WGPURequestAdapterStatus status,
                              WGPUAdapter adapter, const char *message,
                              void *user_data) {
  AdapterRequestUserData *data = (AdapterRequestUserData *)user_data;
  data->request_ended = true;
  if (status != WGPURequestAdapterStatus_Success) {
    LOG_ERROR("WGPU Adapter request failed: %s", message);
    return;
  }

  data->adapter = adapter;
}

WGPUAdapter request_adapter(WGPUInstance instance) {
  WGPURequestAdapterOptions adapter_options = {};
  AdapterRequestUserData user_data = {.adapter = NULL, .request_ended = false};

  wgpuInstanceRequestAdapter(instance, &adapter_options,
                             on_adapter_request_ended, &user_data);

  ASSERT(user_data.request_ended);
  return user_data.adapter;
}

typedef struct {
  WGPUDevice device;
  bool request_ended;
} DeviceRequestUserData;

void on_device_request_ended(WGPURequestDeviceStatus status, WGPUDevice device,
                             const char *message, void *user_data) {
  DeviceRequestUserData *data = (DeviceRequestUserData *)user_data;
  data->request_ended = true;
  if (status != WGPURequestDeviceStatus_Success) {
    LOG_ERROR("WGPU Device request failed: %s", message);
    return;
  }

  data->device = device;
}

WGPUDevice request_device(WGPUAdapter adapter,
                          const WGPUDeviceDescriptor *wgpu_device_descriptor) {
  DeviceRequestUserData user_data = {.device = NULL, .request_ended = false};
  wgpuAdapterRequestDevice(adapter, wgpu_device_descriptor,
                           on_device_request_ended, &user_data);

  ASSERT(user_data.request_ended);
  return user_data.device;
}

WGPURenderPipeline
create_render_pipeline(WGPUDevice device, WGPUShaderModule shader_module,
                       WGPUTextureFormat color_target_format,
                       WGPUBindGroupLayout common_uniforms_bind_group_layout,
                       WGPUBindGroupLayout mesh_uniforms_bind_group_layout,
                       WGPUBindGroupLayout texture_bind_group_layout);

Renderer *renderer_new(SDL_Window *window, Assets *assets,
                       float current_time_secs) {
  LOG_INFO("Initializing renderer...");
  Renderer *renderer = memory_allocate(sizeof(Renderer));
  if (!renderer)
    goto err;

  WGPUInstanceDescriptor wgpu_instance_descriptor = {.nextInChain = NULL};

  renderer->wgpu_instance = wgpuCreateInstance(&wgpu_instance_descriptor);
  if (!renderer->wgpu_instance) {
    LOG_ERROR("WGPUInstance creation failed");
    goto cleanup;
  }

  renderer->wgpu_adapter = request_adapter(renderer->wgpu_instance);
  if (!renderer->wgpu_adapter)
    goto cleanup_instance;

  size_t adapter_feature_count =
      wgpuAdapterEnumerateFeatures(renderer->wgpu_adapter, NULL);
  WGPUFeatureName *adapter_features =
      memory_allocate_array(adapter_feature_count, sizeof(WGPUFeatureName));
  wgpuAdapterEnumerateFeatures(renderer->wgpu_adapter, adapter_features);

  LOG_INFO("Adapter features: ");
  for (size_t i = 0; i < adapter_feature_count; i++) {
    LOG_INFO("\t%d", adapter_features[i]);
  }
  memory_free(adapter_features);

  renderer->wgpu_surface = NULL;
  renderer_initialize_for_window(renderer, window);
  if (!renderer->wgpu_surface) {
    goto cleanup_adapter;
  }

  WGPUSupportedLimits supported_limits = {.nextInChain = NULL};
  wgpuAdapterGetLimits(renderer->wgpu_adapter, &supported_limits);

  WGPURequiredLimits required_limits = {0};
  required_limits.limits.maxVertexAttributes = 3;
  required_limits.limits.maxVertexBuffers = 1;
  required_limits.limits.maxBindGroups = 3;
  required_limits.limits.maxUniformBuffersPerShaderStage = 2;
  required_limits.limits.maxBindingsPerBindGroup = 1;
  required_limits.limits.maxUniformBufferBindingSize = sizeof(CommonUniforms);
  required_limits.limits.maxBufferSize = 160;
  required_limits.limits.maxVertexBufferArrayStride = sizeof(Vertex);
  required_limits.limits.maxInterStageShaderComponents = 3;
  required_limits.limits.maxTextureDimension2D = 800;
  required_limits.limits.maxTextureArrayLayers = 1;
  required_limits.limits.maxSampledTexturesPerShaderStage = 1;
  required_limits.limits.maxSamplersPerShaderStage = 1;
  required_limits.limits.minStorageBufferOffsetAlignment =
      supported_limits.limits.minStorageBufferOffsetAlignment;
  required_limits.limits.minUniformBufferOffsetAlignment =
      supported_limits.limits.minUniformBufferOffsetAlignment;

  WGPUDeviceDescriptor wgpu_device_descriptor = {
      .nextInChain = NULL,
      .label = "device",
      .requiredFeatureCount = 0,
      .requiredLimits = &required_limits,
      .defaultQueue.nextInChain = NULL,
      .defaultQueue.label = "default_queue",
      .requiredFeatures = NULL,
      .deviceLostCallback = NULL,
      .deviceLostUserdata = NULL,
  };
  renderer->wgpu_device =
      request_device(renderer->wgpu_adapter, &wgpu_device_descriptor);
  if (!renderer->wgpu_device) {
    goto cleanup_surface;
  }

  wgpuAdapterGetLimits(renderer->wgpu_adapter, &supported_limits);
  wgpuDeviceGetLimits(renderer->wgpu_device, &supported_limits);
  wgpuDeviceSetUncapturedErrorCallback(renderer->wgpu_device, on_device_error,
                                       NULL);

  WGPUQueue queue = wgpuDeviceGetQueue(renderer->wgpu_device);
  wgpuQueueOnSubmittedWorkDone(queue, on_queue_submitted_work_done, NULL);

  WGPUSurfaceCapabilities surface_capabilities = {0};
  wgpuSurfaceGetCapabilities(renderer->wgpu_surface, renderer->wgpu_adapter,
                             &surface_capabilities);
  renderer->wgpu_render_surface_texture_format =
      surface_capabilities.formats[0];
  WGPUSurfaceConfiguration wgpu_surface_configuration = {
      .width = 800,
      .height = 600,
      .device = renderer->wgpu_device,
      .usage = WGPUTextureUsage_RenderAttachment,
      .format = renderer->wgpu_render_surface_texture_format,
      .presentMode = WGPUPresentMode_Fifo,
      .alphaMode = surface_capabilities.alphaModes[0]};
  wgpuSurfaceConfigure(renderer->wgpu_surface, &wgpu_surface_configuration);

  assets_register_loader(assets, Shader, &shader_asset_loader,
                         &shader_asset_destructor);
  Shader *shader_asset = assets_fetch(assets, Shader, "shader/shader.wgsl");

  WGPUShaderModule shader_module = shader_create_wgpu_shader_module(
      renderer->wgpu_device, "shader", shader_asset->source);

  Image *image = assets_fetch(assets, Image, "sand_texture.png");
  renderer->sand_texture = wgpuDeviceCreateTexture(
      renderer->wgpu_device,
      &(const WGPUTextureDescriptor){.label = "sand_texture",
                                     .dimension = WGPUTextureDimension_2D,
                                     .format = WGPUTextureFormat_RGBA8UnormSrgb,
                                     .viewFormatCount = 0,
                                     .mipLevelCount = 1,
                                     .sampleCount = 1,
                                     .usage = WGPUTextureUsage_TextureBinding |
                                              WGPUTextureUsage_CopyDst,
                                     .size = {16, 16, 1}});
  wgpuQueueWriteTexture(
      queue,
      &(const WGPUImageCopyTexture){.texture = renderer->sand_texture,
                                    .aspect = WGPUTextureAspect_All,
                                    .mipLevel = 0,
                                    .origin = {0, 0, 0}},
      image->data, image->width * image->height * image->bytes_per_pixel,
      &(const WGPUTextureDataLayout){.offset = 0,
                                     .bytesPerRow =
                                         image->bytes_per_pixel * image->width,
                                     .rowsPerImage = image->height},
      &(const WGPUExtent3D){16, 16, 1});
  renderer->sand_texture_bind_group_layout = wgpuDeviceCreateBindGroupLayout(
      renderer->wgpu_device,
      &(const WGPUBindGroupLayoutDescriptor){
          .label = "sand_texture_bind_group_layout",
          .entryCount = 2,
          .entries = (const WGPUBindGroupLayoutEntry[]){
              (WGPUBindGroupLayoutEntry){
                  .binding = 0,
                  .visibility = WGPUShaderStage_Fragment,
                  .texture = {.sampleType = WGPUTextureSampleType_Float,
                              .viewDimension = WGPUTextureViewDimension_2D}},
              (WGPUBindGroupLayoutEntry){
                  .binding = 1,
                  .visibility = WGPUShaderStage_Fragment,
                  .sampler = {.type = WGPUSamplerBindingType_Filtering}}}});

  WGPUTextureView sand_texture_view = wgpuTextureCreateView(
      renderer->sand_texture, &(const WGPUTextureViewDescriptor){
                                  .aspect = WGPUTextureAspect_All,
                                  .baseArrayLayer = 0,
                                  .arrayLayerCount = 1,
                                  .baseMipLevel = 0,
                                  .mipLevelCount = 1,
                                  .dimension = WGPUTextureViewDimension_2D,
                                  .format = WGPUTextureFormat_RGBA8UnormSrgb,
                              });

  WGPUSampler sand_texture_sampler = wgpuDeviceCreateSampler(
      renderer->wgpu_device, &(const WGPUSamplerDescriptor){
                                 .addressModeU = WGPUAddressMode_ClampToEdge,
                                 .addressModeV = WGPUAddressMode_ClampToEdge,
                                 .addressModeW = WGPUAddressMode_ClampToEdge,
                                 .magFilter = WGPUFilterMode_Nearest,
                                 .minFilter = WGPUFilterMode_Nearest,
                                 .mipmapFilter = WGPUMipmapFilterMode_Nearest,
                                 .lodMinClamp = 0.0f,
                                 .lodMaxClamp = 1.0f,
                                 .compare = WGPUCompareFunction_Undefined,
                                 .maxAnisotropy = 1

                             });
  renderer->sand_texture_bind_group = wgpuDeviceCreateBindGroup(
      renderer->wgpu_device,
      &(const WGPUBindGroupDescriptor){
          .label = "sand_texture_bind_group",
          .layout = renderer->sand_texture_bind_group_layout,
          .entryCount = 2,
          .entries = (const WGPUBindGroupEntry[]){
              (WGPUBindGroupEntry){.binding = 0,
                                   .textureView = sand_texture_view},
              (WGPUBindGroupEntry){.binding = 1,
                                   .sampler = sand_texture_sampler}}});

  Vertex *vertices = memory_allocate_array(4, sizeof(Vertex));
  vertices[0] = (Vertex){.position = {-0.5, -0.5, -0.3},
                         .texture_coordinates = {0.0, 1.0}};
  vertices[1] = (Vertex){.position = {0.5, -0.5, -0.3},
                         .texture_coordinates = {1.0, 1.0}};
  vertices[2] =
      (Vertex){.position = {0.5, 0.5, -0.3}, .texture_coordinates = {1.0, 0.0}};
  vertices[3] = (Vertex){.position = {-0.5, 0.5, -0.3},
                         .texture_coordinates = {0.0, 0.0}};

  Index *indices = memory_allocate_array(6, sizeof(Index));
  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 3;

  indices[3] = 3;
  indices[4] = 1;
  indices[5] = 2;

  Mesh mesh = {0};
  mesh_init(&mesh, vertices, 4, indices, 6);
  GPUMesh gpu_mesh = {0};
  gpu_mesh_init(renderer->wgpu_device, queue, &gpu_mesh, &mesh);
  renderer->mesh = gpu_mesh;
  mesh_deinit(&mesh);

  renderer->mesh_transform = TRANSFORM_DEFAULT;
  renderer->mesh_transform.position.z = 10.0;
  renderer->mesh_transform.scale = (v3f){1.0, 1.0, 1.0};
  mat4 m = {0};
  transform_matrix(&renderer->mesh_transform, m);
  mat4_transpose(m);

  renderer->mesh_uniforms = (MeshUniforms){0};
  memcpy(renderer->mesh_uniforms.world_from_local, m,
         16 * sizeof(mat4_value_type));

  renderer->mesh_uniforms_buffer = wgpuDeviceCreateBuffer(
      renderer->wgpu_device,
      &(const WGPUBufferDescriptor){.label = "mesh_uniforms_buffer",
                                    .size = sizeof(MeshUniforms),
                                    .usage = WGPUBufferUsage_CopyDst |
                                             WGPUBufferUsage_Uniform});
  wgpuQueueWriteBuffer(queue, renderer->mesh_uniforms_buffer, 0,
                       &renderer->mesh_uniforms, sizeof(MeshUniforms));
  renderer->mesh_uniforms_bind_group_layout = wgpuDeviceCreateBindGroupLayout(
      renderer->wgpu_device,
      &(const WGPUBindGroupLayoutDescriptor){
          .label = "mesh_uniforms_bind_group_layout",
          .entryCount = 1,
          .entries =
              &(const WGPUBindGroupLayoutEntry){
                  .binding = 0,
                  .buffer =
                      (WGPUBufferBindingLayout){
                          .type = WGPUBufferBindingType_Uniform,
                          .minBindingSize = sizeof(MeshUniforms),
                          .hasDynamicOffset = false},
                  .sampler =
                      (WGPUSamplerBindingLayout){
                          .type = WGPUSamplerBindingType_Undefined},
                  .storageTexture =
                      (WGPUStorageTextureBindingLayout){
                          .access = WGPUStorageTextureAccess_Undefined,
                          .format = WGPUTextureFormat_Undefined,
                          .viewDimension = WGPUTextureViewDimension_Undefined,
                      },
                  .texture =
                      (WGPUTextureBindingLayout){
                          .multisampled = false,
                          .sampleType = WGPUTextureSampleType_Undefined,
                          .viewDimension = WGPUTextureViewDimension_Undefined},
                  .visibility = WGPUShaderStage_Vertex},
      });

  renderer->mesh_uniforms_bind_group = wgpuDeviceCreateBindGroup(
      renderer->wgpu_device,
      &(const WGPUBindGroupDescriptor){
          .label = "mesh_uniforms_bind_group",
          .layout = renderer->mesh_uniforms_bind_group_layout,
          .entryCount = 1,
          .entries = &(const WGPUBindGroupEntry){
              .binding = 0,
              .buffer = renderer->mesh_uniforms_buffer,
              .offset = 0,
              .size = sizeof(MeshUniforms)}});

  Camera camera;
  camera_init_perspective(&camera, 45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
  mat4 projection_matrix = {0};
  memcpy(projection_matrix, camera.projection_matrix, 16 * sizeof(float));
  mat4_transpose(projection_matrix);
  memcpy(renderer->common_uniforms.projection_from_view, projection_matrix,
         16 * sizeof(mat4_value_type));
  renderer->common_uniforms.current_time_secs = current_time_secs;
  renderer->common_uniforms_buffer = wgpuDeviceCreateBuffer(
      renderer->wgpu_device,
      &(const WGPUBufferDescriptor){.label = "common_uniforms_buffer",
                                    .size = sizeof(CommonUniforms),
                                    .usage = WGPUBufferUsage_CopyDst |
                                             WGPUBufferUsage_Uniform,
                                    .mappedAtCreation = false});
  wgpuQueueWriteBuffer(queue, renderer->common_uniforms_buffer, 0,
                       &renderer->common_uniforms, sizeof(CommonUniforms));
  renderer->common_uniforms_bind_group_layout = wgpuDeviceCreateBindGroupLayout(
      renderer->wgpu_device,
      &(const WGPUBindGroupLayoutDescriptor){
          .label = "common_uniforms_bind_group_layout",
          .entryCount = 1,
          .entries = &(const WGPUBindGroupLayoutEntry){
              .binding = 0,
              .buffer =
                  (WGPUBufferBindingLayout){
                      .type = WGPUBufferBindingType_Uniform,
                      .minBindingSize = sizeof(CommonUniforms),
                      .hasDynamicOffset = false},
              .sampler =
                  (WGPUSamplerBindingLayout){
                      .type = WGPUSamplerBindingType_Undefined},
              .storageTexture =
                  (WGPUStorageTextureBindingLayout){
                      .access = WGPUStorageTextureAccess_Undefined,
                      .format = WGPUTextureFormat_Undefined,
                      .viewDimension = WGPUTextureViewDimension_Undefined,
                  },
              .texture =
                  (WGPUTextureBindingLayout){
                      .multisampled = false,
                      .sampleType = WGPUTextureSampleType_Undefined,
                      .viewDimension = WGPUTextureViewDimension_Undefined},
              .visibility = WGPUShaderStage_Vertex}});
  renderer->common_uniforms_bind_group = wgpuDeviceCreateBindGroup(
      renderer->wgpu_device,
      &(const WGPUBindGroupDescriptor){
          .label = "common_uniforms_bind_group",
          .layout = renderer->common_uniforms_bind_group_layout,
          .entryCount = 1,
          .entries =
              &(const WGPUBindGroupEntry){.binding = 0,
                                          .buffer =
                                              renderer->common_uniforms_buffer,
                                          .offset = 0,
                                          .size = sizeof(CommonUniforms)}

      });

  renderer->depth_texture_format = WGPUTextureFormat_Depth24Plus;
  renderer->depth_texture = wgpuDeviceCreateTexture(
      renderer->wgpu_device, &(const WGPUTextureDescriptor){
                                 .dimension = WGPUTextureDimension_2D,
                                 .format = renderer->depth_texture_format,
                                 .mipLevelCount = 1,
                                 .sampleCount = 1,
                                 .size = {800, 600, 1},
                                 .usage = WGPUTextureUsage_RenderAttachment,
                                 .viewFormatCount = 1,
                                 .viewFormats = &renderer->depth_texture_format,
                             });
  renderer->depth_texture_view = wgpuTextureCreateView(
      renderer->depth_texture, &(const WGPUTextureViewDescriptor){
                                   .aspect = WGPUTextureAspect_DepthOnly,
                                   .baseArrayLayer = 0,
                                   .arrayLayerCount = 1,
                                   .baseMipLevel = 0,
                                   .mipLevelCount = 1,
                                   .dimension = WGPUTextureViewDimension_2D,
                                   .format = renderer->depth_texture_format});
  renderer->pipeline = create_render_pipeline(
      renderer->wgpu_device, shader_module, surface_capabilities.formats[0],
      renderer->common_uniforms_bind_group_layout,
      renderer->mesh_uniforms_bind_group_layout,
      renderer->sand_texture_bind_group_layout);

  return renderer;
cleanup_surface:
  wgpuSurfaceRelease(renderer->wgpu_surface);
cleanup_adapter:
  wgpuAdapterRelease(renderer->wgpu_adapter);
cleanup_instance:
  wgpuInstanceRelease(renderer->wgpu_instance);
cleanup:
  memory_free(renderer);
err:
  return NULL;
}

WGPURenderPipeline
create_render_pipeline(WGPUDevice device, WGPUShaderModule shader_module,
                       WGPUTextureFormat color_target_format,
                       WGPUBindGroupLayout common_uniforms_bind_group_layout,
                       WGPUBindGroupLayout mesh_uniforms_bind_group_layout,
                       WGPUBindGroupLayout texture_bind_group_layout) {

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

  WGPUBindGroupLayout bind_group_layouts[] = {common_uniforms_bind_group_layout,
                                              mesh_uniforms_bind_group_layout,
                                              texture_bind_group_layout};
  WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(
      device, &(const WGPUPipelineLayoutDescriptor){.label = "pipeline_layout",
                                                    .bindGroupLayouts =
                                                        bind_group_layouts,
                                                    .bindGroupLayoutCount = 3});

  return wgpuDeviceCreateRenderPipeline(
      device,
      &(WGPURenderPipelineDescriptor){
          .label = "pipeline",
          .nextInChain = NULL,
          .layout = pipeline_layout,
          .vertex = (WGPUVertexState){.nextInChain = NULL,
                                      .bufferCount = 1,
                                      .buffers = &vertex_buffer_layout,
                                      .constantCount = 0,
                                      .constants = NULL,
                                      .entryPoint = "vs_main",
                                      .module = shader_module},
          .fragment =
              &(WGPUFragmentState){
                  .nextInChain = NULL,
                  .constantCount = 0,
                  .constants = NULL,
                  .targets =
                      &(WGPUColorTargetState){
                          .nextInChain = NULL,
                          .format = color_target_format,
                          .blend =
                              &(const WGPUBlendState){
                                  .color =
                                      (WGPUBlendComponent){
                                          .srcFactor = WGPUBlendFactor_SrcAlpha,
                                          .dstFactor =
                                              WGPUBlendFactor_OneMinusSrcAlpha,
                                          .operation = WGPUBlendOperation_Add},
                                  .alpha =
                                      (WGPUBlendComponent){
                                          .srcFactor = WGPUBlendFactor_Zero,
                                          .dstFactor = WGPUBlendFactor_One,
                                          .operation = WGPUBlendOperation_Add}},

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
          .primitive =
              (WGPUPrimitiveState){
                  .nextInChain = NULL,
                  .topology = WGPUPrimitiveTopology_TriangleList,
                  .cullMode = WGPUCullMode_Back,
                  .frontFace = WGPUFrontFace_CCW,
                  .stripIndexFormat = WGPUIndexFormat_Undefined}

      });
}
void renderer_recreate_pipeline(Assets *assets, Renderer *renderer) {
  assets_remove(assets, Shader, "shader/shader.wgsl");
  Shader *shader_asset = assets_fetch(assets, Shader, "shader/shader.wgsl");

  WGPUShaderModuleWGSLDescriptor shader_module_wgsl_descriptor = {
      .chain =
          (WGPUChainedStruct){.next = NULL,
                              .sType = WGPUSType_ShaderModuleWGSLDescriptor},
      .code = shader_asset->source};

  WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(
      renderer->wgpu_device,
      &(const WGPUShaderModuleDescriptor){
          .label = "shader_module",
          .nextInChain = &shader_module_wgsl_descriptor.chain,
          .hintCount = 0,
          .hints = NULL});

  wgpuRenderPipelineRelease(renderer->pipeline);
  renderer->pipeline =
      create_render_pipeline(renderer->wgpu_device, shader_module,
                             renderer->wgpu_render_surface_texture_format,
                             renderer->common_uniforms_bind_group_layout,
                             renderer->mesh_uniforms_bind_group_layout,
                             renderer->sand_texture_bind_group_layout);
}

void renderer_destroy(Renderer *renderer) {
  ASSERT(renderer != NULL);

  wgpuTextureViewRelease(renderer->depth_texture_view);
  wgpuTextureDestroy(renderer->depth_texture);
  wgpuTextureRelease(renderer->depth_texture);
  wgpuDeviceRelease(renderer->wgpu_device);
  wgpuSurfaceRelease(renderer->wgpu_surface);
  wgpuAdapterRelease(renderer->wgpu_adapter);
  wgpuInstanceRelease(renderer->wgpu_instance);
  memory_free(renderer);
}

void renderer_initialize_for_window(Renderer *renderer, SDL_Window *window) {
  SDL_SysWMinfo info;
  SDL_VERSION(&info.version);
  SDL_GetWindowWMInfo(window, &info);

  // FIXME this will only work on X11 this needs to be expanded to support
  // other platforms such as Windows, check wgpu-native examples
  WGPUSurfaceDescriptorFromXlibWindow x11_surface_descriptor = {
      .chain = {.next = NULL,
                .sType = WGPUSType_SurfaceDescriptorFromXlibWindow},
      .display = info.info.x11.display,
      .window = info.info.x11.window};

  WGPUSurfaceDescriptor surface_descriptor = {
      .label = NULL, .nextInChain = &x11_surface_descriptor.chain};

  WGPUSurface wgpu_surface =
      wgpuInstanceCreateSurface(renderer->wgpu_instance, &surface_descriptor);

  renderer->wgpu_surface = wgpu_surface;
}

void renderer_render(Renderer *renderer, float current_time_secs) {
  WGPUSurfaceTexture surface_texture;
  wgpuSurfaceGetCurrentTexture(renderer->wgpu_surface, &surface_texture);
  switch (surface_texture.status) {
  case WGPUSurfaceGetCurrentTextureStatus_Success:
    break;
  case WGPUSurfaceGetCurrentTextureStatus_Timeout:
  case WGPUSurfaceGetCurrentTextureStatus_Outdated:
  case WGPUSurfaceGetCurrentTextureStatus_Lost:
    // FIXME Reconfigure surface with the potentially new window size
    abort();
    break;
  case WGPUSurfaceGetCurrentTextureStatus_OutOfMemory:
  case WGPUSurfaceGetCurrentTextureStatus_DeviceLost:
  case WGPUSurfaceGetCurrentTextureStatus_Force32:
    LOG_ERROR("get_current_texture status=%#.8x", surface_texture.status);
    exit(1);
  }
  WGPUTextureView surface_texture_view =
      wgpuTextureCreateView(surface_texture.texture, NULL);

  WGPUQueue queue = wgpuDeviceGetQueue(renderer->wgpu_device);

  renderer->common_uniforms.current_time_secs = current_time_secs;
  wgpuQueueWriteBuffer(queue, renderer->common_uniforms_buffer, 0,
                       &renderer->common_uniforms, sizeof(CommonUniforms));

  Quaternion q = {1, {0}};
  quaternion_set_to_axis_angle(&q, &(const v3f){0.0, 1.0, 0.0}, 0.05);
  quaternion_mul(&renderer->mesh_transform.rotation, &q);
  transform_matrix(&renderer->mesh_transform,
                   renderer->mesh_uniforms.world_from_local);
  mat4_transpose(renderer->mesh_uniforms.world_from_local);
  wgpuQueueWriteBuffer(queue, renderer->mesh_uniforms_buffer, 0,
                       &renderer->mesh_uniforms, sizeof(MeshUniforms));

  WGPUCommandEncoderDescriptor command_encoder_descriptor = {
      .nextInChain = NULL, .label = "command_encoder"};
  WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(
      renderer->wgpu_device, &command_encoder_descriptor);
  WGPURenderPassEncoder render_pass_encoder = wgpuCommandEncoderBeginRenderPass(
      command_encoder,
      &(const WGPURenderPassDescriptor){
          .label = "render_pass",
          .colorAttachmentCount = 1,
          .colorAttachments =
              &(const WGPURenderPassColorAttachment){
                  .view = surface_texture_view,
                  .loadOp = WGPULoadOp_Clear,
                  .storeOp = WGPUStoreOp_Store,
                  .clearValue =
                      (WGPUColor){.r = 0.1, .g = 0.2, .b = 0.5, .a = 1.0},
              },
          .depthStencilAttachment =
              &(const WGPURenderPassDepthStencilAttachment){
                  .view = renderer->depth_texture_view,
                  .depthClearValue = 1.0f,
                  .depthLoadOp = WGPULoadOp_Clear,
                  .depthStoreOp = WGPUStoreOp_Store,
                  .depthReadOnly = false,
                  .stencilClearValue = 0,
                  .stencilLoadOp = WGPULoadOp_Clear,
                  .stencilStoreOp = WGPUStoreOp_Store,
                  .stencilReadOnly = true}});

  wgpuRenderPassEncoderSetPipeline(render_pass_encoder, renderer->pipeline);
  gpu_mesh_bind(render_pass_encoder, &renderer->mesh);
  wgpuRenderPassEncoderSetBindGroup(
      render_pass_encoder, 0, renderer->common_uniforms_bind_group, 0, NULL);
  wgpuRenderPassEncoderSetBindGroup(
      render_pass_encoder, 1, renderer->mesh_uniforms_bind_group, 0, NULL);
  wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 2,
                                    renderer->sand_texture_bind_group, 0, NULL);
  wgpuRenderPassEncoderDrawIndexed(render_pass_encoder,
                                   renderer->mesh.index_count, 1, 0, 0, 0);
  wgpuRenderPassEncoderEnd(render_pass_encoder);

  WGPUCommandBufferDescriptor command_buffer_descriptor = {
      .nextInChain = NULL, .label = "command_buffer"};
  WGPUCommandBuffer command_buffer =
      wgpuCommandEncoderFinish(command_encoder, &command_buffer_descriptor);

  wgpuQueueSubmit(queue, 1, &command_buffer);
  wgpuSurfacePresent(renderer->wgpu_surface);

  wgpuCommandBufferRelease(command_buffer);
  wgpuRenderPassEncoderRelease(render_pass_encoder);
  wgpuCommandEncoderRelease(command_encoder);
  wgpuTextureViewRelease(surface_texture_view);
  wgpuTextureRelease(surface_texture.texture);
}
