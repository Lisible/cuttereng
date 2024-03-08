#include "renderer.h"
#include "../assert.h"
#include "../hash.h"
#include "../image.h"
#include "../log.h"
#include "../math/matrix.h"
#include "../memory.h"
#include "default_pipeline.h"
#include "font.h"
#include "material.h"
#include "mesh.h"
#include "render_graph.h"
#include "shader.h"
#include "src/filesystem.h"
#include "src/graphics/light.h"
#include "src/vec.h"
#include "webgpu/webgpu.h"
#include <SDL2/SDL_syswm.h>

void HashTableRenderPipeline_destructor(Allocator *allocator, void *value) {
  (void)allocator;
  wgpuRenderPipelineRelease(value);
}
DEF_HASH_TABLE(WGPURenderPipeline, HashTableRenderPipeline,
               HashTableRenderPipeline_destructor)

void HashTableShaderModule_destructor(Allocator *allocator, void *value) {
  (void)allocator;
  wgpuShaderModuleRelease(value);
}
DEF_HASH_TABLE(WGPUShaderModule, HashTableShaderModule,
               HashTableShaderModule_destructor)
void HashTableTexture_destructor(Allocator *allocator, void *value) {
  (void)allocator;
  wgpuTextureDestroy(value);
  wgpuTextureRelease(value);
}
DEF_HASH_TABLE(WGPUTexture, HashTableTexture, HashTableTexture_destructor)

void HashTableMaterial_destructor(Allocator *allocator, void *value) {
  (void)allocator;
  gpu_material_destroy(allocator, value);
}
DEF_HASH_TABLE(GPUMaterial *, HashTableMaterial, HashTableMaterial_destructor)

DEF_VEC(DrawCommand, DrawCommandQueue, 1000 * sizeof(DrawCommand))

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

void load_shader_modules(Allocator *allocator,
                         HashTableShaderModule *shader_modules,
                         WGPUDevice device, Assets *assets);
void load_textures(Allocator *allocator, HashTableTexture *textures,
                   WGPUDevice device, WGPUQueue queue, Assets *assets);
void load_materials(Allocator *allocator, WGPUDevice device,
                    HashTableMaterial *materials, HashTableTexture *textures,
                    WGPUBindGroupLayout material_bind_group_layout,
                    Assets *assets);

void log_adapter_features(WGPUAdapter adapter, Allocator *allocator);
void get_adapter_required_limits(WGPUAdapter adapter,
                                 WGPUSupportedLimits *supported_limits,
                                 WGPURequiredLimits *out_required_limits);
void configure_surface(RendererContext *ctx);
void initialize_resources(Renderer *renderer, Assets *assets, WGPUQueue queue);
WGPUTexture create_depth_texture(WGPUDevice device,
                                 WGPUTextureFormat *depth_texture_format);
void initialize_common_uniforms(CommonUniforms *uniforms);
WGPUBuffer create_common_uniforms_buffer(WGPUDevice device);
WGPUBindGroupLayout create_common_uniforms_bind_group_layout(WGPUDevice device);
WGPUBindGroup create_common_uniforms_bind_group(
    WGPUDevice device, WGPUBindGroupLayout common_uniforms_bind_group_layout,
    WGPUBuffer common_uniforms_buffer);

WGPUBuffer create_mesh_uniforms_buffer(WGPUDevice device);
WGPUBindGroupLayout create_mesh_uniforms_bind_group_layout(WGPUDevice device);
WGPUBindGroup create_mesh_uniforms_bind_group(
    WGPUDevice device, WGPUBindGroupLayout mesh_uniforms_bind_group_layout,
    WGPUBuffer mesh_uniforms_buffer);
WGPUBindGroupLayout create_material_bind_group_layout(WGPUDevice device);

Renderer *renderer_new(Allocator *allocator, SDL_Window *window,
                       Assets *assets) {
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
  if (!renderer->ctx.wgpu_adapter) {
    goto cleanup_instance;
  }
  log_adapter_features(renderer->ctx.wgpu_adapter, renderer->allocator);

  renderer->ctx.wgpu_surface = NULL;
  renderer_initialize_for_window(renderer, window);
  if (!renderer->ctx.wgpu_surface) {
    goto cleanup_adapter;
  }

  WGPUSupportedLimits supported_limits = {0};
  WGPURequiredLimits required_limits = {0};
  get_adapter_required_limits(renderer->ctx.wgpu_adapter, &supported_limits,
                              &required_limits);

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
  renderer->ctx.wgpu_limits = required_limits;

  WGPUQueue queue = wgpuDeviceGetQueue(renderer->ctx.wgpu_device);
  wgpuQueueOnSubmittedWorkDone(queue, on_queue_submitted_work_done, NULL);
  renderer->ctx.surface_size.width = 800;
  renderer->ctx.surface_size.height = 600;

  configure_surface(&renderer->ctx);
  renderer->ctx.depth_texture_format = WGPUTextureFormat_Depth32Float;
  // renderer->ctx.resolution_factor = 0.4;
  renderer->ctx.resolution_factor = 1.0;
  initialize_resources(renderer, assets, queue);
  renderer->render_pipeline =
      renderer_default_render_pipeline_create(allocator, renderer, assets);
  if (!renderer->render_pipeline) {
    LOG_ERROR("Couldn't create default render pipeline");
    goto cleanup_surface;
  }

  wgpuQueueRelease(queue);

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

void load_bitmap_fonts(Allocator *allocator, Assets *assets) {
  DirectoryListing *dl =
      filesystem_list_files_in_directory(allocator, "assets/fonts");
  for (size_t i = 0; i < dl->entry_count; i++) {
    char font_path[sizeof(FONT_DIR) + MAX_FILENAME_LENGTH] = FONT_DIR;
    strcat(font_path, dl->entries[i]);
    LOG_DEBUG("Loading font %s", dl->entries[i]);
    AssetHandle font_handle;
    if (!assets_load(assets, BitmapFont, font_path, &font_handle)) {
      LOG_ERROR("Couldn't load font %s", dl->entries[i]);
      continue;
    }
    BitmapFont *font = assets_get(assets, BitmapFont, font_handle);
    (void)font;
    // TODO something idk
  }
  filesystem_directory_listing_destroy(allocator, dl);
}
Material *load_material(Assets *assets, char *material_path) {
  AssetHandle material_handle;
  if (!assets_load(assets, Material, material_path, &material_handle)) {
    return NULL;
  }

  return assets_get(assets, Material, material_handle);
}

WGPUShaderModule create_shader_module(WGPUDevice device, Assets *assets,
                                      char *shader_module_identifier) {
  AssetHandle shader_handle;
  if (!assets_load(assets, Shader, shader_module_identifier, &shader_handle)) {
    return NULL;
  }
  Shader *shader = assets_get(assets, Shader, shader_handle);
  WGPUShaderModuleWGSLDescriptor shader_module_wgsl_descriptor = {
      .chain =
          (WGPUChainedStruct){.sType = WGPUSType_ShaderModuleWGSLDescriptor},
      .code = shader->source};
  return wgpuDeviceCreateShaderModule(
      device, &(const WGPUShaderModuleDescriptor){
                  .label = shader_module_identifier,
                  .nextInChain = &shader_module_wgsl_descriptor.chain});
}

void load_shader_modules(Allocator *allocator,
                         HashTableShaderModule *shader_modules,
                         WGPUDevice device, Assets *assets) {
  DirectoryListing *dl =
      filesystem_list_files_in_directory(allocator, "assets/shaders");
  for (size_t i = 0; i < dl->entry_count; i++) {
    char shader_path[sizeof(SHADER_DIR) + MAX_FILENAME_LENGTH] = SHADER_DIR;
    strcat(shader_path, dl->entries[i]);
    LOG_DEBUG("Creating shader module %s", dl->entries[i]);
    WGPUShaderModule shader_module =
        create_shader_module(device, assets, shader_path);
    if (!shader_module) {
      LOG_ERROR("Couldn't create shader module");
    } else {
      HashTableShaderModule_set(shader_modules, dl->entries[i], shader_module);
    }
  }
  filesystem_directory_listing_destroy(allocator, dl);
}

WGPUTexture load_texture(Allocator *allocator, WGPUDevice device,
                         WGPUQueue queue, Assets *assets,
                         AssetHandle texture_handle) {
  (void)allocator;
  Image *image = assets_get(assets, Image, texture_handle);
  WGPUTextureFormat texture_format = WGPUTextureFormat_RGBA8UnormSrgb;
  WGPUTexture texture = wgpuDeviceCreateTexture(
      device,
      &(const WGPUTextureDescriptor){
          .label = "texture",
          .dimension = WGPUTextureDimension_2D,
          .size = {image->width, image->height, 1},
          .format = texture_format,
          .usage = WGPUTextureUsage_RenderAttachment |
                   WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding,
          .viewFormatCount = 1,
          .viewFormats = &texture_format,
          .mipLevelCount = 1,
          .sampleCount = 1});
  wgpuQueueWriteTexture(
      queue,
      &(const WGPUImageCopyTexture){.texture = texture,
                                    .aspect = WGPUTextureAspect_All},
      image->data, image->width * image->height * image->bytes_per_pixel,
      &(const WGPUTextureDataLayout){.bytesPerRow =
                                         image->width * image->bytes_per_pixel,
                                     .rowsPerImage = image->height},
      &(const WGPUExtent3D){image->width, image->height, 1});
  return texture;
}

void renderer_destroy(Renderer *renderer) {
  ASSERT(renderer != NULL);

  renderer_default_render_pipeline_destroy(renderer->allocator,
                                           renderer->render_pipeline);
  HashTableRenderPipeline_destroy(renderer->resources.pipelines);
  wgpuBindGroupLayoutRelease(renderer->resources.material_bind_group_layout);
  wgpuBindGroupLayoutRelease(
      renderer->resources.mesh_uniforms_bind_group_layout);
  wgpuBindGroupRelease(renderer->resources.mesh_uniforms_bind_group);
  wgpuBindGroupLayoutRelease(
      renderer->resources.common_uniforms_bind_group_layout);
  wgpuBindGroupRelease(renderer->resources.common_uniforms_bind_group);
  wgpuBufferDestroy(renderer->resources.mesh_uniforms_buffer);
  wgpuBufferRelease(renderer->resources.mesh_uniforms_buffer);
  wgpuBufferDestroy(renderer->resources.common_uniforms_buffer);
  wgpuBufferRelease(renderer->resources.common_uniforms_buffer);

  VEC_FOR_EACH(&renderer->draw_commands, command, DrawCommand,
               { DrawCommand_deinit(renderer->allocator, command); });
  DrawCommandQueue_deinit(&renderer->draw_commands);

  wgpuTextureDestroy(renderer->resources.depth_texture);
  wgpuTextureRelease(renderer->resources.depth_texture);
  wgpuDeviceRelease(renderer->ctx.wgpu_device);
  wgpuSurfaceRelease(renderer->ctx.wgpu_surface);
  wgpuAdapterRelease(renderer->ctx.wgpu_adapter);
  wgpuInstanceRelease(renderer->ctx.wgpu_instance);
  allocator_free(renderer->allocator, renderer);
}

void renderer_draw_mesh(Renderer *renderer, Transform *transform,
                        AssetHandle mesh_handle, AssetHandle material_handle) {
  ASSERT(renderer != NULL);
  ASSERT(transform != NULL);
  DrawCommand draw_command;
  draw_command.mesh_handle = mesh_handle;
  draw_command.material_handle = material_handle;
  draw_command.uses_shader_material = false;
  memcpy(&draw_command.transform, transform, sizeof(Transform));
  DrawCommandQueue_append(&renderer->draw_commands, &draw_command, 1);
}
void renderer_draw_mesh_with_shader_material(Renderer *renderer,
                                             Transform *transform,
                                             AssetHandle mesh_handle,
                                             AssetHandle material_handle) {
  ASSERT(renderer != NULL);
  ASSERT(transform != NULL);
  DrawCommand draw_command;
  draw_command.mesh_handle = mesh_handle;
  draw_command.material_handle = material_handle;
  draw_command.uses_shader_material = false;
  memcpy(&draw_command.transform, transform, sizeof(Transform));
  DrawCommandQueue_append(&renderer->draw_commands, &draw_command, 1);
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

void load_mesh_in_cache_if_required(Renderer *renderer, WGPUQueue queue,
                                    Assets *assets, AssetHandle mesh_handle) {
  ASSERT(renderer != NULL);
  ASSERT(assets != NULL);
  if (ResourceCaches_has_mesh(&renderer->resources.resource_caches,
                              mesh_handle)) {
    return;
  }

  Mesh *mesh = assets_get(assets, Mesh, mesh_handle);
  GPUMesh *gpu_mesh = allocator_allocate(renderer->allocator, sizeof(GPUMesh));
  gpu_mesh_init(renderer->ctx.wgpu_device, queue, gpu_mesh, mesh);
  ResourceCaches_set_mesh(&renderer->resources.resource_caches, mesh_handle,
                          gpu_mesh);
}

void load_texture_in_cache_if_required(Renderer *renderer, WGPUQueue queue,
                                       Assets *assets,
                                       AssetHandle texture_handle) {
  ASSERT(renderer != NULL);
  ASSERT(assets != NULL);
  if (ResourceCaches_has_texture(&renderer->resources.resource_caches,
                                 texture_handle)) {
    return;
  }

  WGPUTexture texture =
      load_texture(renderer->allocator, renderer->ctx.wgpu_device, queue,
                   assets, texture_handle);
  ResourceCaches_set_texture(&renderer->resources.resource_caches,
                             texture_handle, texture);
}

void load_material_in_cache_if_required(Renderer *renderer, WGPUQueue queue,
                                        Assets *assets,
                                        AssetHandle material_handle) {
  ASSERT(renderer != NULL);
  ASSERT(assets != NULL);
  if (ResourceCaches_has_material(&renderer->resources.resource_caches,
                                  material_handle)) {
    return;
  }

  Material *material = assets_get(assets, Material, material_handle);
  load_texture_in_cache_if_required(renderer, queue, assets,
                                    material->base_color);
  load_texture_in_cache_if_required(renderer, queue, assets, material->normal);

  GPUMaterial *gpu_material = gpu_material_create(
      renderer->allocator, &renderer->resources.resource_caches,
      renderer->ctx.wgpu_device, renderer->resources.material_bind_group_layout,
      material);
  ResourceCaches_set_material(&renderer->resources.resource_caches,
                              material_handle, gpu_material);
}

void renderer_render(Allocator *frame_allocator, Renderer *renderer,
                     Assets *assets, float current_time_secs) {
  WGPUQueue wgpu_queue = wgpuDeviceGetQueue(renderer->ctx.wgpu_device);
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

  // Ensures meshes and materials are loaded
  VEC_FOR_EACH(&renderer->draw_commands, command, DrawCommand, {
    load_mesh_in_cache_if_required(renderer, wgpu_queue, assets,
                                   command->mesh_handle);
    load_material_in_cache_if_required(renderer, wgpu_queue, assets,
                                       command->material_handle);
  });

  WGPUTextureView surface_texture_view =
      wgpuTextureCreateView(surface_texture.texture, NULL);

  RenderGraph render_graph;
  render_graph_init(&render_graph);

  renderer->render_pipeline->fn(
      &renderer->ctx, &renderer->resources,
      renderer->render_pipeline->pipeline_static_state,
      renderer->render_pipeline->pipeline_state, &renderer->draw_commands,
      frame_allocator, &render_graph, wgpu_queue, surface_texture_view,
      current_time_secs);

  WGPUCommandEncoderDescriptor command_encoder_descriptor = {
      .nextInChain = NULL, .label = "command_encoder"};
  WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(
      renderer->ctx.wgpu_device, &command_encoder_descriptor);
  render_graph_execute(&render_graph, command_encoder, &renderer->resources,
                       &renderer->draw_commands);
  WGPUCommandBufferDescriptor command_buffer_descriptor = {
      .nextInChain = NULL, .label = "command_buffer"};
  WGPUCommandBuffer command_buffer =
      wgpuCommandEncoderFinish(command_encoder, &command_buffer_descriptor);
  wgpuQueueSubmit(wgpu_queue, 1, &command_buffer);
  wgpuSurfacePresent(renderer->ctx.wgpu_surface);

  render_graph_deinit(&render_graph);
  renderer->render_pipeline->cleanup_fn(
      renderer->render_pipeline->pipeline_state);
  wgpuQueueRelease(wgpu_queue);
  wgpuCommandBufferRelease(command_buffer);
  wgpuCommandEncoderRelease(command_encoder);
  wgpuTextureRelease(surface_texture.texture);

  VEC_FOR_EACH(&renderer->draw_commands, command, DrawCommand,
               { DrawCommand_deinit(renderer->allocator, command); });
  DrawCommandQueue_clear(&renderer->draw_commands);
  renderer->resources.light_count = 0;
}

void GBuffer_init(GBuffer *g_buffer, WGPUDevice device,
                  RenderGraph *render_graph, u32 width, u32 height) {
  ASSERT(g_buffer != NULL);
  ASSERT(render_graph != NULL);
  g_buffer->base_color = render_graph_create_texture(
      render_graph, RenderGraphResourceUsage_ColorAttachment, device,
      WGPUTextureFormat_RGBA8Unorm, width, height);
  g_buffer->normal = render_graph_create_texture(
      render_graph, RenderGraphResourceUsage_ColorAttachment, device,
      WGPUTextureFormat_RGBA16Float, width, height);
  g_buffer->position = render_graph_create_texture(
      render_graph, RenderGraphResourceUsage_ColorAttachment, device,
      WGPUTextureFormat_RGBA16Float, width, height);
  g_buffer->directional_light_space_depth_map = render_graph_create_texture(
      render_graph, RenderGraphResourceUsage_ColorAttachment, device,
      WGPUTextureFormat_RGBA8Unorm, width, height);
}
WGPUBindGroupLayout GBuffer_create_bind_group_layout(WGPUDevice device) {
  return wgpuDeviceCreateBindGroupLayout(
      device,
      &(const WGPUBindGroupLayoutDescriptor){
          .entries =
              (const WGPUBindGroupLayoutEntry[]){
                  (WGPUBindGroupLayoutEntry){
                      .binding = 0,
                      .visibility = WGPUShaderStage_Fragment,
                      .texture = {.sampleType = WGPUTextureSampleType_Float,
                                  .viewDimension =
                                      WGPUTextureViewDimension_2D}},
                  (WGPUBindGroupLayoutEntry){
                      .binding = 1,
                      .visibility = WGPUShaderStage_Fragment,
                      .sampler = {.type = WGPUSamplerBindingType_Filtering}},
                  (WGPUBindGroupLayoutEntry){
                      .binding = 2,
                      .visibility = WGPUShaderStage_Fragment,
                      .texture = {.sampleType = WGPUTextureSampleType_Float,
                                  .viewDimension =
                                      WGPUTextureViewDimension_2D}},
                  (WGPUBindGroupLayoutEntry){
                      .binding = 3,
                      .visibility = WGPUShaderStage_Fragment,
                      .sampler = {.type = WGPUSamplerBindingType_Filtering}},
                  (WGPUBindGroupLayoutEntry){
                      .binding = 4,
                      .visibility = WGPUShaderStage_Fragment,
                      .texture = {.sampleType = WGPUTextureSampleType_Float,
                                  .viewDimension =
                                      WGPUTextureViewDimension_2D}},
                  (WGPUBindGroupLayoutEntry){
                      .binding = 5,
                      .visibility = WGPUShaderStage_Fragment,
                      .sampler = {.type = WGPUSamplerBindingType_Filtering}},
                  (WGPUBindGroupLayoutEntry){
                      .binding = 6,
                      .visibility = WGPUShaderStage_Fragment,
                      .texture = {.sampleType =
                                      WGPUTextureSampleType_UnfilterableFloat,
                                  .viewDimension =
                                      WGPUTextureViewDimension_2D}},
                  (WGPUBindGroupLayoutEntry){
                      .binding = 7,
                      .visibility = WGPUShaderStage_Fragment,
                      .sampler = {.type =
                                      WGPUSamplerBindingType_NonFiltering}}},
          .entryCount = 8});
}
WGPUBindGroup
GBuffer_create_bind_group(GBuffer *g_buffer, RenderGraph *render_graph,
                          WGPUDevice device,
                          WGPUBindGroupLayout g_buffer_bind_group_layout) {
  return wgpuDeviceCreateBindGroup(
      device,
      &(const WGPUBindGroupDescriptor){
          .layout = g_buffer_bind_group_layout,
          .entries =
              (const WGPUBindGroupEntry[]){
                  (WGPUBindGroupEntry){
                      .binding = 0,
                      .textureView =
                          render_graph->resources[g_buffer->base_color.id]
                              .owned_texture.texture_view},
                  (WGPUBindGroupEntry){
                      .binding = 1,
                      .sampler =
                          render_graph->resources[g_buffer->base_color.id]
                              .owned_texture.texture_sampler},
                  (WGPUBindGroupEntry){
                      .binding = 2,
                      .textureView =
                          render_graph->resources[g_buffer->normal.id]
                              .owned_texture.texture_view},
                  (WGPUBindGroupEntry){
                      .binding = 3,
                      .sampler = render_graph->resources[g_buffer->normal.id]
                                     .owned_texture.texture_sampler},
                  (WGPUBindGroupEntry){
                      .binding = 4,
                      .textureView =
                          render_graph->resources[g_buffer->position.id]
                              .owned_texture.texture_view},
                  (WGPUBindGroupEntry){
                      .binding = 5,
                      .sampler = render_graph->resources[g_buffer->position.id]
                                     .owned_texture.texture_sampler},
                  (WGPUBindGroupEntry){
                      .binding = 6,
                      .textureView =
                          render_graph
                              ->resources
                                  [g_buffer->directional_light_space_depth_map
                                       .id]
                              .owned_texture.texture_view},
                  (WGPUBindGroupEntry){
                      .binding = 7,
                      .sampler =
                          render_graph
                              ->resources
                                  [g_buffer->directional_light_space_depth_map
                                       .id]
                              .owned_texture.texture_sampler}},
          .entryCount = 8});
}

void renderer_set_view_projection(Renderer *renderer, mat4 view_projection) {
  ASSERT(renderer != NULL);
  mat4 view_projection_matrix = {0};
  memcpy(view_projection_matrix, view_projection, 16 * sizeof(mat4_value_type));
  mat4_transpose(view_projection_matrix);
  memcpy(renderer->resources.common_uniforms.projection_from_view,
         view_projection_matrix, 16 * sizeof(mat4_value_type));

  mat4 inverse_projection_matrix = {0};
  mat4_inverse(view_projection_matrix, inverse_projection_matrix);
  memcpy(renderer->resources.common_uniforms.inverse_projection_from_view,
         view_projection_matrix, 16 * sizeof(mat4_value_type));
}
void renderer_add_light(Renderer *renderer, const Light *light) {
  ASSERT(renderer != NULL);
  ASSERT(light != NULL);
  ASSERT(renderer->resources.light_count < MAX_LIGHT_COUNT);
  renderer->resources.lights[renderer->resources.light_count] = *light;
  renderer->resources.light_count++;
}

void renderer_set_view_position(Renderer *renderer, v3f *view_position) {
  renderer->resources.common_uniforms.view_position = *view_position;
}
void renderer_clear_caches(Renderer *renderer) {
  ASSERT(renderer != NULL);
  HashTableRenderPipeline_clear(renderer->resources.pipelines);
  ResourceCaches_clear(&renderer->resources.resource_caches);
}

void initialize_common_uniforms(CommonUniforms *uniforms) {
  uniforms->current_time_secs = 0;
  uniforms->view_position = (v3f){0};

  mat4 identity_matrix = {0};
  mat4_set_to_identity(identity_matrix);
  memcpy(uniforms->projection_from_view, identity_matrix,
         16 * sizeof(mat4_value_type));
  memcpy(uniforms->inverse_projection_from_view, identity_matrix,
         16 * sizeof(mat4_value_type));
  memcpy(uniforms->light_space_projection_from_view_from_local, identity_matrix,
         16 * sizeof(mat4_value_type));

  uniforms->directional_light = (DirectionalLight){0};
}

WGPUBuffer create_common_uniforms_buffer(WGPUDevice device) {
  return wgpuDeviceCreateBuffer(
      device, &(const WGPUBufferDescriptor){.label = "common_uniforms_buffer",
                                            .size = sizeof(CommonUniforms),
                                            .usage = WGPUBufferUsage_CopyDst |
                                                     WGPUBufferUsage_Uniform,
                                            .mappedAtCreation = false});
}

void log_adapter_features(WGPUAdapter adapter, Allocator *allocator) {
  size_t adapter_feature_count = wgpuAdapterEnumerateFeatures(adapter, NULL);
  WGPUFeatureName *adapter_features = allocator_allocate_array(
      allocator, adapter_feature_count, sizeof(WGPUFeatureName));
  wgpuAdapterEnumerateFeatures(adapter, adapter_features);

  LOG_INFO("Adapter features: ");
  for (size_t i = 0; i < adapter_feature_count; i++) {
    LOG_INFO("\t%d", adapter_features[i]);
  }
  allocator_free(allocator, adapter_features);
}

void get_adapter_required_limits(WGPUAdapter adapter,
                                 WGPUSupportedLimits *supported_limits,
                                 WGPURequiredLimits *out_required_limits) {
  ASSERT(out_required_limits != NULL);
  wgpuAdapterGetLimits(adapter, supported_limits);

  out_required_limits->limits.maxVertexAttributes = 3;
  out_required_limits->limits.maxVertexBuffers = 1;
  out_required_limits->limits.maxBindGroups = 3;
  out_required_limits->limits.maxUniformBuffersPerShaderStage = 2;
  out_required_limits->limits.maxBindingsPerBindGroup = 8;
  out_required_limits->limits.maxUniformBufferBindingSize =
      sizeof(CommonUniforms);
  out_required_limits->limits.maxBufferSize = 192000000;
  out_required_limits->limits.maxVertexBufferArrayStride = sizeof(Vertex);
  out_required_limits->limits.maxInterStageShaderComponents = 9;
  out_required_limits->limits.maxTextureDimension2D = 4096;
  out_required_limits->limits.maxTextureArrayLayers = 1;
  out_required_limits->limits.maxSampledTexturesPerShaderStage = 4;
  out_required_limits->limits.maxSamplersPerShaderStage = 4;
  out_required_limits->limits.maxDynamicUniformBuffersPerPipelineLayout = 1;
  out_required_limits->limits.minStorageBufferOffsetAlignment =
      supported_limits->limits.minStorageBufferOffsetAlignment;
  out_required_limits->limits.minUniformBufferOffsetAlignment =
      supported_limits->limits.minUniformBufferOffsetAlignment;
}

void configure_surface(RendererContext *ctx) {
  WGPUSurfaceCapabilities surface_capabilities = {0};
  wgpuSurfaceGetCapabilities(ctx->wgpu_surface, ctx->wgpu_adapter,
                             &surface_capabilities);
  ctx->wgpu_render_surface_texture_format = surface_capabilities.formats[0];
  WGPUSurfaceConfiguration wgpu_surface_configuration = {
      .width = ctx->surface_size.width,
      .height = ctx->surface_size.height,
      .device = ctx->wgpu_device,
      .usage = WGPUTextureUsage_RenderAttachment,
      .format = ctx->wgpu_render_surface_texture_format,
      .presentMode = WGPUPresentMode_Fifo,
      .alphaMode = surface_capabilities.alphaModes[0]};
  wgpuSurfaceCapabilitiesFreeMembers(surface_capabilities);
  wgpuSurfaceConfigure(ctx->wgpu_surface, &wgpu_surface_configuration);
}

WGPUBuffer create_mesh_uniforms_buffer(WGPUDevice device) {
  return wgpuDeviceCreateBuffer(
      device, &(const WGPUBufferDescriptor){
                  .label = "mesh_uniforms_buffer",
                  .size = MAX_MESH_DRAW_PER_FRAME * sizeof(MeshUniforms),
                  .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform});
}

WGPUBindGroupLayout create_mesh_uniforms_bind_group_layout(WGPUDevice device) {
  return wgpuDeviceCreateBindGroupLayout(
      device, &(const WGPUBindGroupLayoutDescriptor){
                  .label = "mesh_uniforms_bind_group_layout",
                  .entryCount = 1,
                  .entries =
                      &(const WGPUBindGroupLayoutEntry){
                          .binding = 0,
                          .buffer =
                              (WGPUBufferBindingLayout){
                                  .type = WGPUBufferBindingType_Uniform,
                                  .minBindingSize = sizeof(MeshUniforms),
                                  .hasDynamicOffset = true},
                          .visibility = WGPUShaderStage_Vertex |
                                        WGPUShaderStage_Fragment},
              });
}

WGPUBindGroup create_mesh_uniforms_bind_group(
    WGPUDevice device, WGPUBindGroupLayout mesh_uniforms_bind_group_layout,
    WGPUBuffer mesh_uniforms_buffer) {
  return wgpuDeviceCreateBindGroup(
      device, &(const WGPUBindGroupDescriptor){
                  .label = "mesh_uniforms_bind_group",
                  .layout = mesh_uniforms_bind_group_layout,
                  .entryCount = 1,
                  .entries = &(const WGPUBindGroupEntry){
                      .binding = 0,
                      .buffer = mesh_uniforms_buffer,
                      .offset = 0,
                      .size = sizeof(MeshUniforms)}});
}
WGPUBindGroupLayout
create_common_uniforms_bind_group_layout(WGPUDevice device) {

  return wgpuDeviceCreateBindGroupLayout(
      device, &(const WGPUBindGroupLayoutDescriptor){
                  .label = "common_uniforms_bind_group_layout",
                  .entryCount = 1,
                  .entries = &(const WGPUBindGroupLayoutEntry){
                      .binding = 0,
                      .buffer =
                          (WGPUBufferBindingLayout){
                              .type = WGPUBufferBindingType_Uniform,
                              .minBindingSize = sizeof(CommonUniforms),
                              .hasDynamicOffset = false},
                      .visibility =
                          WGPUShaderStage_Vertex | WGPUShaderStage_Fragment}});
}

WGPUBindGroup create_common_uniforms_bind_group(
    WGPUDevice device, WGPUBindGroupLayout common_uniforms_bind_group_layout,
    WGPUBuffer common_uniforms_buffer) {
  return wgpuDeviceCreateBindGroup(
      device,
      &(const WGPUBindGroupDescriptor){
          .label = "common_uniforms_bind_group",
          .layout = common_uniforms_bind_group_layout,
          .entryCount = 1,
          .entries =
              &(const WGPUBindGroupEntry){.binding = 0,
                                          .buffer = common_uniforms_buffer,
                                          .offset = 0,
                                          .size = sizeof(CommonUniforms)}

      });
}

WGPUTexture create_depth_texture(WGPUDevice device,
                                 WGPUTextureFormat *depth_texture_format) {
  return wgpuDeviceCreateTexture(device,
                                 &(const WGPUTextureDescriptor){
                                     .dimension = WGPUTextureDimension_2D,
                                     .format = *depth_texture_format,
                                     .mipLevelCount = 1,
                                     .sampleCount = 1,
                                     .size = {800, 600, 1},
                                     .usage = WGPUTextureUsage_RenderAttachment,
                                     .viewFormatCount = 1,
                                     .viewFormats = depth_texture_format,
                                 });
}

WGPUBindGroupLayout create_material_bind_group_layout(WGPUDevice device) {
  return wgpuDeviceCreateBindGroupLayout(
      device,
      &(const WGPUBindGroupLayoutDescriptor){
          .entryCount = 4,
          .entries = (const WGPUBindGroupLayoutEntry[]){
              (const WGPUBindGroupLayoutEntry){
                  .binding = 0,
                  .visibility = WGPUShaderStage_Fragment,
                  .texture = {.sampleType = WGPUTextureSampleType_Float,
                              .viewDimension = WGPUTextureViewDimension_2D}},
              (const WGPUBindGroupLayoutEntry){
                  .binding = 1,
                  .visibility = WGPUShaderStage_Fragment,
                  .sampler = {.type = WGPUSamplerBindingType_Filtering}},
              (const WGPUBindGroupLayoutEntry){
                  .binding = 2,
                  .visibility = WGPUShaderStage_Fragment,
                  .texture = {.sampleType = WGPUTextureSampleType_Float,
                              .viewDimension = WGPUTextureViewDimension_2D}},
              (const WGPUBindGroupLayoutEntry){
                  .binding = 3,
                  .visibility = WGPUShaderStage_Fragment,
                  .sampler = {.type = WGPUSamplerBindingType_Filtering}},
          }});
}

void initialize_resources(Renderer *renderer, Assets *assets, WGPUQueue queue) {
  ASSERT(renderer != NULL);
  ASSERT(assets != NULL);
  ASSERT(queue != NULL);

  renderer->resources.light_count = 0;
  renderer->resources.depth_texture = create_depth_texture(
      renderer->ctx.wgpu_device, &renderer->ctx.depth_texture_format);

  DrawCommandQueue_init(renderer->allocator, &renderer->draw_commands);
  initialize_common_uniforms(&renderer->resources.common_uniforms);
  renderer->resources.common_uniforms_buffer =
      create_common_uniforms_buffer(renderer->ctx.wgpu_device);
  wgpuQueueWriteBuffer(queue, renderer->resources.common_uniforms_buffer, 0,
                       &renderer->resources.common_uniforms,
                       sizeof(CommonUniforms));
  renderer->resources.common_uniforms_bind_group_layout =
      create_common_uniforms_bind_group_layout(renderer->ctx.wgpu_device);
  renderer->resources.common_uniforms_bind_group =
      create_common_uniforms_bind_group(
          renderer->ctx.wgpu_device,
          renderer->resources.common_uniforms_bind_group_layout,
          renderer->resources.common_uniforms_buffer);
  renderer->resources.material_bind_group_layout =
      create_material_bind_group_layout(renderer->ctx.wgpu_device);

  renderer->resources.mesh_uniform_count = 0;
  renderer->resources.mesh_uniforms_buffer =
      create_mesh_uniforms_buffer(renderer->ctx.wgpu_device);
  renderer->resources.mesh_uniforms_bind_group_layout =
      create_mesh_uniforms_bind_group_layout(renderer->ctx.wgpu_device);
  renderer->resources.mesh_uniforms_bind_group =
      create_mesh_uniforms_bind_group(
          renderer->ctx.wgpu_device,
          renderer->resources.mesh_uniforms_bind_group_layout,
          renderer->resources.mesh_uniforms_buffer);

  renderer->resources.pipelines =
      HashTableRenderPipeline_create(renderer->allocator, 32);
  ResourceCaches_init(&renderer->resources.resource_caches);
  assets_set_destructor(assets, Mesh, &mesh_destructor);

  assets_register_loader(assets, Shader, &shader_asset_loader,
                         &shader_asset_destructor);
  assets_register_loader(assets, Material, &material_loader,
                         &material_destructor);
  assets_register_loader(assets, BitmapFont, &bitmap_font_loader,
                         &bitmap_font_destructor);
}

void DrawCommand_deinit(Allocator *allocator, DrawCommand *command) {
  ASSERT(allocator != NULL);
  ASSERT(command != NULL);
}
