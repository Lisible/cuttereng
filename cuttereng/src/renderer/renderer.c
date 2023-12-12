#include "renderer.h"
#include "../assert.h"
#include "../asset.h"
#include "../log.h"
#include "../memory.h"
#include <SDL2/SDL_syswm.h>

typedef struct {
  char *source;
} ShaderAsset;

void *shader_asset_loader(const char *path) {
  ShaderAsset *shader_asset = memory_allocate(sizeof(ShaderAsset));
  shader_asset->source = asset_read_file_to_string(path);
  return shader_asset;
}
void shader_asset_destructor(void *asset) {
  ShaderAsset *shader_asset = asset;
  memory_free(shader_asset->source);
  memory_free(shader_asset);
}

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
                       WGPUTextureFormat color_target_format);

Renderer *renderer_new(SDL_Window *window, Assets *assets) {
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
  required_limits.limits.maxVertexAttributes = 2;
  required_limits.limits.maxVertexBuffers = 1;
  required_limits.limits.maxBufferSize = 6 * 5 * sizeof(float);
  required_limits.limits.maxVertexBufferArrayStride = 5 * sizeof(float);
  required_limits.limits.maxInterStageShaderComponents = 3;
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

  assets_register_loader(assets, ShaderAsset, shader_asset_loader,
                         shader_asset_destructor);
  ShaderAsset *shader_asset =
      assets_fetch(assets, ShaderAsset, "shader/shader.wgsl");

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

  renderer->pipeline = create_render_pipeline(
      renderer->wgpu_device, shader_module, surface_capabilities.formats[0]);

  float vertex_data[] = {-0.5, -0.5,  1.0, 0.0, 0.0,   0.5, -0.5, 0.0,
                         1.0,  0.0,   0.0, 0.5, 1.0,   1.0, 0.0,  -0.55,
                         -0.5, 1.0,   1.0, 0.0, -0.05, 0.5, 1.0,  0.0,
                         1.0,  -0.55, 0.5, 0.0, 1.0,   1.0};
  renderer->vertex_buffer_length = sizeof(vertex_data) / sizeof(float);
  renderer->vertex_buffer = wgpuDeviceCreateBuffer(
      renderer->wgpu_device,
      &(const WGPUBufferDescriptor){
          .label = "vertex buffer",
          .mappedAtCreation = false,
          .nextInChain = NULL,
          .size = renderer->vertex_buffer_length * sizeof(float),
          .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex});
  wgpuQueueWriteBuffer(queue, renderer->vertex_buffer, 0, vertex_data,
                       renderer->vertex_buffer_length * sizeof(float));

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
                       WGPUTextureFormat color_target_format) {

  WGPUVertexAttribute vertex_attributes[] = {
      {.format = WGPUVertexFormat_Float32x2, .offset = 0, .shaderLocation = 0},
      {.format = WGPUVertexFormat_Float32x3,
       .offset = 2 * sizeof(float),
       .shaderLocation = 1}};
  WGPUVertexBufferLayout vertex_buffer_layout = {
      .attributeCount = 2,
      .attributes = vertex_attributes,
      .arrayStride = 5 * sizeof(float),
      .stepMode = WGPUVertexStepMode_Vertex};

  return wgpuDeviceCreateRenderPipeline(
      device,
      &(WGPURenderPipelineDescriptor){
          .label = "pipeline",
          .nextInChain = NULL,
          .layout = NULL,
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
          .depthStencil = NULL,
          .multisample =
              (WGPUMultisampleState){.nextInChain = NULL,
                                     .count = 1,
                                     .mask = ~0u,
                                     .alphaToCoverageEnabled = false},
          .primitive =
              (WGPUPrimitiveState){
                  .nextInChain = NULL,
                  .topology = WGPUPrimitiveTopology_TriangleList,
                  .cullMode = WGPUCullMode_None,
                  .frontFace = WGPUFrontFace_CCW,
                  .stripIndexFormat = WGPUIndexFormat_Undefined}

      });
}
void renderer_recreate_pipeline(Assets *assets, Renderer *renderer) {
  assets_remove(assets, ShaderAsset, "shader/shader.wgsl");
  ShaderAsset *shader_asset =
      assets_fetch(assets, ShaderAsset, "shader/shader.wgsl");

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
                             renderer->wgpu_render_surface_texture_format);
}

void renderer_destroy(Renderer *renderer) {
  ASSERT(renderer != NULL);

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

void renderer_render(Renderer *renderer) {
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
      });

  wgpuRenderPassEncoderSetPipeline(render_pass_encoder, renderer->pipeline);
  wgpuRenderPassEncoderSetVertexBuffer(
      render_pass_encoder, 0, renderer->vertex_buffer, 0,
      renderer->vertex_buffer_length * sizeof(float));
  wgpuRenderPassEncoderDraw(render_pass_encoder,
                            renderer->vertex_buffer_length / 5, 1, 0, 0);
  wgpuRenderPassEncoderEnd(render_pass_encoder);

  WGPUCommandBufferDescriptor command_buffer_descriptor = {
      .nextInChain = NULL, .label = "command_buffer"};
  WGPUCommandBuffer command_buffer =
      wgpuCommandEncoderFinish(command_encoder, &command_buffer_descriptor);

  WGPUQueue queue = wgpuDeviceGetQueue(renderer->wgpu_device);
  wgpuQueueSubmit(queue, 1, &command_buffer);
  wgpuSurfacePresent(renderer->wgpu_surface);

  wgpuCommandBufferRelease(command_buffer);
  wgpuRenderPassEncoderRelease(render_pass_encoder);
  wgpuCommandEncoderRelease(command_encoder);
  wgpuTextureViewRelease(surface_texture_view);
  wgpuTextureRelease(surface_texture.texture);
}
