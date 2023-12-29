#include "renderer.h"
#include "../assert.h"
#include "../hash.h"
#include "../image.h"
#include "../log.h"
#include "../math/matrix.h"
#include "../math/quaternion.h"
#include "../memory.h"
#include "../transform.h"
#include "render_graph.h"
#include "shader.h"
#include "webgpu/webgpu.h"
#include <SDL2/SDL_syswm.h>
#include <math.h>

DEF_HASH_TABLE(WGPURenderPipeline, HashTableRenderPipeline,
               HashTable_noop_destructor)
DEF_HASH_TABLE(WGPUShaderModule, HashTableShaderModule,
               HashTable_noop_destructor)

void on_queue_submitted_work_done(WGPUQueueWorkDoneStatus status,
                                  void *user_data) {
  (void)user_data;
  LOG_TRACE("Queued work finished with status: %d", status);
}

void on_device_error(WGPUErrorType error_type, const char *message,
                     void *user_data) {
  (void)user_data;
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
  WGPURequestAdapterOptions adapter_options = {0};
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

Renderer *renderer_new(Allocator *allocator, SDL_Window *window, Assets *assets,
                       float current_time_secs) {
  LOG_INFO("Initializing renderer...");
  Renderer *renderer = allocator_allocate(allocator, (sizeof(Renderer)));
  if (!renderer)
    goto err;

  renderer->allocator = allocator;

  WGPUInstanceDescriptor wgpu_instance_descriptor = {.nextInChain = NULL};
  renderer->ctx.wgpu_instance = wgpuCreateInstance(&wgpu_instance_descriptor);
  if (!renderer->ctx.wgpu_instance) {
    LOG_ERROR("WGPUInstance creation failed");
    goto cleanup;
  }

  renderer->ctx.wgpu_adapter = request_adapter(renderer->ctx.wgpu_instance);
  if (!renderer->ctx.wgpu_adapter)
    goto cleanup_instance;

  size_t adapter_feature_count =
      wgpuAdapterEnumerateFeatures(renderer->ctx.wgpu_adapter, NULL);
  WGPUFeatureName *adapter_features = allocator_allocate_array(
      allocator, adapter_feature_count, sizeof(WGPUFeatureName));
  wgpuAdapterEnumerateFeatures(renderer->ctx.wgpu_adapter, adapter_features);

  LOG_INFO("Adapter features: ");
  for (size_t i = 0; i < adapter_feature_count; i++) {
    LOG_INFO("\t%d", adapter_features[i]);
  }
  allocator_free(allocator, adapter_features);

  renderer->ctx.wgpu_surface = NULL;
  renderer_initialize_for_window(renderer, window);
  if (!renderer->ctx.wgpu_surface) {
    goto cleanup_adapter;
  }

  WGPUSupportedLimits supported_limits = {.nextInChain = NULL};
  wgpuAdapterGetLimits(renderer->ctx.wgpu_adapter, &supported_limits);

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
  renderer->ctx.wgpu_device =
      request_device(renderer->ctx.wgpu_adapter, &wgpu_device_descriptor);
  if (!renderer->ctx.wgpu_device) {
    goto cleanup_surface;
  }

  wgpuAdapterGetLimits(renderer->ctx.wgpu_adapter, &supported_limits);
  wgpuDeviceGetLimits(renderer->ctx.wgpu_device, &supported_limits);
  wgpuDeviceSetUncapturedErrorCallback(renderer->ctx.wgpu_device,
                                       on_device_error, NULL);

  WGPUQueue queue = wgpuDeviceGetQueue(renderer->ctx.wgpu_device);
  wgpuQueueOnSubmittedWorkDone(queue, on_queue_submitted_work_done, NULL);

  WGPUSurfaceCapabilities surface_capabilities = {0};
  wgpuSurfaceGetCapabilities(renderer->ctx.wgpu_surface,
                             renderer->ctx.wgpu_adapter, &surface_capabilities);
  renderer->ctx.wgpu_render_surface_texture_format =
      surface_capabilities.formats[0];
  WGPUSurfaceConfiguration wgpu_surface_configuration = {
      .width = 800,
      .height = 600,
      .device = renderer->ctx.wgpu_device,
      .usage = WGPUTextureUsage_RenderAttachment,
      .format = renderer->ctx.wgpu_render_surface_texture_format,
      .presentMode = WGPUPresentMode_Fifo,
      .alphaMode = surface_capabilities.alphaModes[0]};
  wgpuSurfaceConfigure(renderer->ctx.wgpu_surface, &wgpu_surface_configuration);

  assets_register_loader(assets, Shader, &shader_asset_loader,
                         &shader_asset_destructor);

  Vertex *vertices = allocator_allocate_array(allocator, 4, sizeof(Vertex));
  vertices[0] = (Vertex){.position = {-0.5, -0.5, -0.3},
                         .texture_coordinates = {0.0, 1.0}};
  vertices[1] = (Vertex){.position = {0.5, -0.5, -0.3},
                         .texture_coordinates = {1.0, 1.0}};
  vertices[2] =
      (Vertex){.position = {0.5, 0.5, -0.3}, .texture_coordinates = {1.0, 0.0}};
  vertices[3] = (Vertex){.position = {-0.5, 0.5, -0.3},
                         .texture_coordinates = {0.0, 0.0}};

  Index *indices = allocator_allocate_array(allocator, 6, sizeof(Index));
  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 3;

  indices[3] = 3;
  indices[4] = 1;
  indices[5] = 2;

  renderer->resources.mesh_uniforms = (MeshUniforms){0};
  renderer->resources.mesh_uniforms_buffer = wgpuDeviceCreateBuffer(
      renderer->ctx.wgpu_device,
      &(const WGPUBufferDescriptor){.label = "mesh_uniforms_buffer",
                                    .size = sizeof(MeshUniforms),
                                    .usage = WGPUBufferUsage_CopyDst |
                                             WGPUBufferUsage_Uniform});
  wgpuQueueWriteBuffer(queue, renderer->resources.mesh_uniforms_buffer, 0,
                       &renderer->resources.mesh_uniforms,
                       sizeof(MeshUniforms));
  renderer->resources
      .mesh_uniforms_bind_group_layout = wgpuDeviceCreateBindGroupLayout(
      renderer->ctx.wgpu_device,
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

  renderer->resources.mesh_uniforms_bind_group = wgpuDeviceCreateBindGroup(
      renderer->ctx.wgpu_device,
      &(const WGPUBindGroupDescriptor){
          .label = "mesh_uniforms_bind_group",
          .layout = renderer->resources.mesh_uniforms_bind_group_layout,
          .entryCount = 1,
          .entries = &(const WGPUBindGroupEntry){
              .binding = 0,
              .buffer = renderer->resources.mesh_uniforms_buffer,
              .offset = 0,
              .size = sizeof(MeshUniforms)}});

  Camera camera;
  camera_init_perspective(&camera, 45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
  mat4 projection_matrix = {0};
  memcpy(projection_matrix, camera.projection_matrix, 16 * sizeof(float));
  mat4_transpose(projection_matrix);
  memcpy(renderer->resources.common_uniforms.projection_from_view,
         projection_matrix, 16 * sizeof(mat4_value_type));
  renderer->resources.common_uniforms.current_time_secs = current_time_secs;
  renderer->resources.common_uniforms_buffer = wgpuDeviceCreateBuffer(
      renderer->ctx.wgpu_device,
      &(const WGPUBufferDescriptor){.label = "common_uniforms_buffer",
                                    .size = sizeof(CommonUniforms),
                                    .usage = WGPUBufferUsage_CopyDst |
                                             WGPUBufferUsage_Uniform,
                                    .mappedAtCreation = false});
  wgpuQueueWriteBuffer(queue, renderer->resources.common_uniforms_buffer, 0,
                       &renderer->resources.common_uniforms,
                       sizeof(CommonUniforms));
  renderer->resources.common_uniforms_bind_group_layout =
      wgpuDeviceCreateBindGroupLayout(
          renderer->ctx.wgpu_device,
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
  renderer->resources.common_uniforms_bind_group = wgpuDeviceCreateBindGroup(
      renderer->ctx.wgpu_device,
      &(const WGPUBindGroupDescriptor){
          .label = "common_uniforms_bind_group",
          .layout = renderer->resources.common_uniforms_bind_group_layout,
          .entryCount = 1,
          .entries =
              &(const WGPUBindGroupEntry){
                  .binding = 0,
                  .buffer = renderer->resources.common_uniforms_buffer,
                  .offset = 0,
                  .size = sizeof(CommonUniforms)}

      });

  renderer->ctx.depth_texture_format = WGPUTextureFormat_Depth24Plus;
  renderer->resources.depth_texture = wgpuDeviceCreateTexture(
      renderer->ctx.wgpu_device,
      &(const WGPUTextureDescriptor){
          .dimension = WGPUTextureDimension_2D,
          .format = renderer->ctx.depth_texture_format,
          .mipLevelCount = 1,
          .sampleCount = 1,
          .size = {800, 600, 1},
          .usage = WGPUTextureUsage_RenderAttachment,
          .viewFormatCount = 1,
          .viewFormats = &renderer->ctx.depth_texture_format,
      });
  renderer->resources.pipelines = HashTableRenderPipeline_create(allocator, 32);
  renderer->resources.shader_modules =
      HashTableShaderModule_create(allocator, 32);

  return renderer;
cleanup_surface:
  wgpuSurfaceRelease(renderer->ctx.wgpu_surface);
cleanup_adapter:
  wgpuAdapterRelease(renderer->ctx.wgpu_adapter);
cleanup_instance:
  wgpuInstanceRelease(renderer->ctx.wgpu_instance);
cleanup:
  allocator_free(allocator, renderer);
err:
  return NULL;
}

void renderer_destroy(Renderer *renderer) {
  ASSERT(renderer != NULL);

  wgpuTextureDestroy(renderer->resources.depth_texture);
  wgpuTextureRelease(renderer->resources.depth_texture);
  wgpuDeviceRelease(renderer->ctx.wgpu_device);
  wgpuSurfaceRelease(renderer->ctx.wgpu_surface);
  wgpuAdapterRelease(renderer->ctx.wgpu_adapter);
  wgpuInstanceRelease(renderer->ctx.wgpu_instance);
  allocator_free(renderer->allocator, renderer);
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

  WGPUSurface wgpu_surface = wgpuInstanceCreateSurface(
      renderer->ctx.wgpu_instance, &surface_descriptor);

  renderer->ctx.wgpu_surface = wgpu_surface;
}

void renderer_render(Allocator *frame_allocator, Renderer *renderer,
                     Assets *assets, float current_time_secs) {

  WGPUSurfaceTexture surface_texture;
  wgpuSurfaceGetCurrentTexture(renderer->ctx.wgpu_surface, &surface_texture);
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

  RenderGraph render_graph;
  render_graph_init(&render_graph);
  render_graph_add_pass(&render_graph, assets, &renderer->ctx,
                        &renderer->resources,
                        &(const RenderPassDescriptor){
                            .identifier = "mesh_pass",
                            .shader_module_identifier = "shader/shader.wgsl",
                            .uses_vertex_buffer = true});

  WGPUQueue queue = wgpuDeviceGetQueue(renderer->ctx.wgpu_device);

  renderer->resources.common_uniforms.current_time_secs = current_time_secs;
  wgpuQueueWriteBuffer(queue, renderer->resources.common_uniforms_buffer, 0,
                       &renderer->resources.common_uniforms,
                       sizeof(CommonUniforms));

  WGPUCommandEncoderDescriptor command_encoder_descriptor = {
      .nextInChain = NULL, .label = "command_encoder"};
  WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(
      renderer->ctx.wgpu_device, &command_encoder_descriptor);

  //  TODO render_graph_execute(render_graph); // create the wgpurenderpass with
  //  the pipeline and execute the dispatch function if there is one

  WGPUCommandBufferDescriptor command_buffer_descriptor = {
      .nextInChain = NULL, .label = "command_buffer"};
  WGPUCommandBuffer command_buffer =
      wgpuCommandEncoderFinish(command_encoder, &command_buffer_descriptor);
  wgpuQueueSubmit(queue, 1, &command_buffer);
  wgpuSurfacePresent(renderer->ctx.wgpu_surface);

  wgpuCommandBufferRelease(command_buffer);
  wgpuCommandEncoderRelease(command_encoder);
  wgpuTextureViewRelease(surface_texture_view);
  wgpuTextureRelease(surface_texture.texture);
}
