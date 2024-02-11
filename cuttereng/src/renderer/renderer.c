#include "renderer.h"
#include "../assert.h"
#include "../hash.h"
#include "../image.h"
#include "../log.h"
#include "../math/matrix.h"
#include "../memory.h"
#include "material.h"
#include "mesh.h"
#include "render_graph.h"
#include "shader.h"
#include "src/filesystem.h"
#include "src/graphics/light.h"
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
  renderer->ctx.resolution_factor = 0.4;

  initialize_resources(renderer, assets, queue);
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

Material *load_material(Assets *assets, char *material_path) {
  return assets_fetch(assets, Material, material_path);
}

void load_materials(Allocator *allocator, WGPUDevice device,
                    HashTableMaterial *materials, HashTableTexture *textures,
                    WGPUBindGroupLayout material_bind_group_layout,
                    Assets *assets) {
  DirectoryListing *dl =
      filesystem_list_files_in_directory(allocator, "assets/materials");
  for (size_t i = 0; i < dl->entry_count; i++) {
    char material_path[sizeof(MATERIAL_DIR) + MAX_FILENAME_LENGTH] =
        MATERIAL_DIR;
    strcat(material_path, dl->entries[i]);
    LOG_DEBUG("Loading material %s", dl->entries[i]);
    Material *material = load_material(assets, material_path);
    if (!material) {
      LOG_ERROR("Couldn't load material %s", dl->entries[i]);
      continue;
    }

    LOG_ERROR("material->base_color: %s", material->base_color);

    GPUMaterial *gpu_material = gpu_material_create(
        allocator, textures, device, material_bind_group_layout, material);
    if (!gpu_material) {
      LOG_ERROR("Couldn't create material binding data");
      material_destroy(allocator, material);
      continue;
    }
    HashTableMaterial_set(materials, dl->entries[i], gpu_material);
  }
  filesystem_directory_listing_destroy(allocator, dl);
}

WGPUShaderModule create_shader_module(WGPUDevice device, Assets *assets,
                                      char *shader_module_identifier) {
  Shader *shader = assets_fetch(assets, Shader, shader_module_identifier);
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

WGPUTexture load_texture(WGPUDevice device, WGPUQueue queue, Assets *assets,
                         char *texture_path) {
  Image *image = assets_fetch(assets, Image, texture_path);
  // TODO set based on the image format
  WGPUTextureFormat texture_format = WGPUTextureFormat_RGBA8UnormSrgb;
  WGPUTexture texture = wgpuDeviceCreateTexture(
      device,
      &(const WGPUTextureDescriptor){
          .label = texture_path,
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

void load_textures(Allocator *allocator, HashTableTexture *textures,
                   WGPUDevice device, WGPUQueue queue, Assets *assets) {
  (void)textures;
  (void)device;
  (void)assets;
  DirectoryListing *dl =
      filesystem_list_files_in_directory(allocator, "assets/textures");
  for (size_t i = 0; i < dl->entry_count; i++) {
    char texture_path[sizeof(TEXTURE_DIR) + MAX_FILENAME_LENGTH] = TEXTURE_DIR;
    strcat(texture_path, dl->entries[i]);
    LOG_DEBUG("Loading texture %s", dl->entries[i]);
    WGPUTexture texture = load_texture(device, queue, assets, texture_path);
    if (!texture) {
      LOG_ERROR("Texture loading failed");
    } else {
      HashTableTexture_set(textures, dl->entries[i], texture);
    }
  }
  filesystem_directory_listing_destroy(allocator, dl);
}

void renderer_destroy(Renderer *renderer) {
  ASSERT(renderer != NULL);

  HashTableRenderPipeline_destroy(renderer->resources.pipelines);
  HashTableShaderModule_destroy(renderer->resources.shader_modules);
  HashTableTexture_destroy(renderer->resources.textures);
  HashTableMaterial_destroy(renderer->resources.materials);
  gpu_mesh_deinit(&renderer->resources.cube_mesh);
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

  for (size_t draw_command_index = 0;
       draw_command_index < renderer->draw_commands.length;
       draw_command_index++) {
    allocator_free(
        renderer->allocator,
        renderer->draw_commands.data[draw_command_index].material_identifier);
  }
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
                        char *material_identifier) {
  ASSERT(renderer != NULL);
  ASSERT(transform != NULL);
  DrawCommand draw_command;
  size_t material_identifier_length = strlen(material_identifier) + 1;
  draw_command.material_identifier =
      allocator_allocate(renderer->allocator, material_identifier_length);
  memcpy(draw_command.material_identifier, material_identifier,
         material_identifier_length);
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

void skybox_pass_dispatch(WGPURenderPassEncoder render_pass_encoder,
                          RendererResources *res,
                          DrawCommandQueue *command_queue, GPUMesh *mesh,
                          u32 mesh_stride, void *custom_data) {
  (void)custom_data;
  (void)res;
  (void)command_queue;
  (void)mesh;
  (void)mesh_stride;
  wgpuRenderPassEncoderDraw(render_pass_encoder, 3, 1, 0, 0);
}

void debug_grid_pass_dispatch(WGPURenderPassEncoder render_pass_encoder,
                              RendererResources *res,
                              DrawCommandQueue *command_queue, GPUMesh *mesh,
                              u32 mesh_stride, void *custom_data) {
  (void)custom_data;
  (void)res;
  (void)command_queue;
  (void)mesh;
  (void)mesh_stride;
  wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 0,
                                    res->common_uniforms_bind_group, 0, NULL);
  wgpuRenderPassEncoderDraw(render_pass_encoder, 6, 1, 0, 0);
}

void mesh_pass_dispatch(WGPURenderPassEncoder render_pass_encoder,
                        RendererResources *res, DrawCommandQueue *command_queue,
                        GPUMesh *mesh, u32 mesh_stride, void *custom_data) {
  (void)custom_data;
  wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 0,
                                    res->common_uniforms_bind_group, 0, NULL);
  gpu_mesh_bind(render_pass_encoder, mesh);
  for (size_t command_index = 0;
       command_index < DrawCommandQueue_length(command_queue);
       command_index++) {
    DrawCommand *command = &command_queue->data[command_index];
    u32 stride = mesh_stride * command_index;
    wgpuRenderPassEncoderSetBindGroup(
        render_pass_encoder, 1, res->mesh_uniforms_bind_group, 1, &stride);

    GPUMaterial *material =
        HashTableMaterial_get(res->materials, command->material_identifier);
    wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 2,
                                      material->bind_group, 0, 0);
    wgpuRenderPassEncoderDraw(render_pass_encoder, mesh->vertex_count, 1, 0, 0);
  }
}

void directional_light_space_depth_map_pass_dispatch(
    WGPURenderPassEncoder render_pass_encoder, RendererResources *res,
    DrawCommandQueue *command_queue, GPUMesh *mesh, u32 mesh_stride,
    void *custom_data) {
  (void)custom_data;
  wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 0,
                                    res->common_uniforms_bind_group, 0, NULL);
  gpu_mesh_bind(render_pass_encoder, mesh);
  for (size_t command_index = 0;
       command_index < DrawCommandQueue_length(command_queue);
       command_index++) {
    u32 stride = mesh_stride * command_index;
    wgpuRenderPassEncoderSetBindGroup(
        render_pass_encoder, 1, res->mesh_uniforms_bind_group, 1, &stride);
    wgpuRenderPassEncoderDraw(render_pass_encoder, mesh->vertex_count, 1, 0, 0);
  }
}

typedef struct {
  WGPUBindGroup g_buffer_bind_group;
} DeferredLightingPassData;

void deferred_lighting_pass_dispatch(WGPURenderPassEncoder render_pass_encoder,
                                     RendererResources *res,
                                     DrawCommandQueue *command_queue,
                                     GPUMesh *mesh, u32 mesh_stride,
                                     void *custom_data) {
  (void)command_queue;
  (void)mesh;
  (void)mesh_stride;
  DeferredLightingPassData *pass_data = custom_data;
  wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 0,
                                    res->common_uniforms_bind_group, 0, NULL);
  wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 1,
                                    pass_data->g_buffer_bind_group, 0, NULL);
  wgpuRenderPassEncoderDraw(render_pass_encoder, 6, 1, 0, 0);
}

typedef struct {
  WGPUBindGroup deferred_lighting_result_bind_group;
} CompositionPassData;

void composition_pass_dispatch(WGPURenderPassEncoder render_pass_encoder,
                               RendererResources *res,
                               DrawCommandQueue *command_queue, GPUMesh *mesh,
                               u32 mesh_stride, void *custom_data) {
  (void)res;
  (void)command_queue;
  (void)mesh;
  (void)mesh_stride;
  CompositionPassData *pass_data = custom_data;
  wgpuRenderPassEncoderSetBindGroup(
      render_pass_encoder, 0, pass_data->deferred_lighting_result_bind_group, 0,
      NULL);
  wgpuRenderPassEncoderDraw(render_pass_encoder, 6, 1, 0, 0);
}

uint32_t ceilToNextMultiple(uint32_t value, uint32_t step) {
  uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
  return step * divide_and_ceil;
}
void renderer_render(Allocator *frame_allocator, Renderer *renderer,
                     float current_time_secs) {
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
  WGPUTextureView surface_texture_view =
      wgpuTextureCreateView(surface_texture.texture, NULL);

  RenderGraph render_graph;
  render_graph_init(&render_graph);
  RenderGraphResourceHandle surface_texture_handle =
      render_graph_register_texture_view(
          &render_graph, renderer->ctx.wgpu_device, surface_texture_view,
          RenderGraphResourceUsage_ColorAttachment,
          WGPUTextureFormat_BGRA8UnormSrgb);

  u32 internal_width =
      renderer->ctx.resolution_factor * renderer->ctx.surface_size.width;
  u32 internal_height =
      renderer->ctx.resolution_factor * renderer->ctx.surface_size.height;
  RenderGraphResourceHandle depth_texture = render_graph_create_texture(
      &render_graph, RenderGraphResourceUsage_DepthStencilAttachment,
      renderer->ctx.wgpu_device, WGPUTextureFormat_Depth32Float, internal_width,
      internal_height);

  GBuffer g_buffer;
  GBuffer_init(&g_buffer, renderer->ctx.wgpu_device, &render_graph,
               internal_width, internal_height);
  WGPUBindGroupLayout g_buffer_bind_group_layout =
      GBuffer_create_bind_group_layout(renderer->ctx.wgpu_device);
  WGPUBindGroup g_buffer_bind_group = GBuffer_create_bind_group(
      &g_buffer, &render_graph, renderer->ctx.wgpu_device,
      g_buffer_bind_group_layout);

  render_graph_add_pass(
      frame_allocator, &render_graph, &renderer->ctx, &renderer->resources,
      &(const RenderPassDescriptor){
          .identifier = "g_buffer_pass",
          .bind_group_layouts =
              {renderer->resources.common_uniforms_bind_group_layout,
               renderer->resources.mesh_uniforms_bind_group_layout,
               renderer->resources.material_bind_group_layout},
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

  render_graph_add_pass(
      frame_allocator, &render_graph, &renderer->ctx, &renderer->resources,
      &(const RenderPassDescriptor){
          .identifier = "directional_light_space_depth_map_pass",
          .bind_group_layouts =
              {renderer->resources.common_uniforms_bind_group_layout,
               renderer->resources.mesh_uniforms_bind_group_layout},
          .bind_group_layout_count = 2,
          .render_attachments = {(RenderPassRenderAttachment){
              .type = RenderPassRenderAttachmentType_DepthAttachment,
              .render_attachment_handle =
                  &g_buffer.directional_light_space_depth_map}},
          .render_attachment_count = 1,
          .uses_vertex_buffer = true,
          .cull_mode = CullMode_Front,
          .shader_module = {.module_identifier =
                                "directional_light_space_depth_map.wgsl",
                            .has_no_fragment_shader = true},
          .dispatch_fn = directional_light_space_depth_map_pass_dispatch});

  RenderGraphResourceHandle deferred_lighting_result =
      render_graph_create_texture(
          &render_graph, RenderGraphResourceUsage_ColorAttachment,
          renderer->ctx.wgpu_device, WGPUTextureFormat_RGBA8UnormSrgb,
          renderer->ctx.surface_size.width, renderer->ctx.surface_size.height);
  DeferredLightingPassData deferred_lighting_pass_data = {
      .g_buffer_bind_group = g_buffer_bind_group};
  render_graph_add_pass(
      frame_allocator, &render_graph, &renderer->ctx, &renderer->resources,
      &(const RenderPassDescriptor){
          .identifier = "deferred_lighting_pass",
          .bind_group_layouts =
              {renderer->resources.common_uniforms_bind_group_layout,
               g_buffer_bind_group_layout},
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
          .pass_data = &deferred_lighting_pass_data,
          .dispatch_fn = deferred_lighting_pass_dispatch});

  render_graph_add_pass(
      frame_allocator, &render_graph, &renderer->ctx, &renderer->resources,
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

  WGPUBindGroupLayout deferred_lighting_result_bind_group_layout =
      wgpuDeviceCreateBindGroupLayout(
          renderer->ctx.wgpu_device,
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

  WGPUBindGroup deferred_lighting_result_bind_group = wgpuDeviceCreateBindGroup(
      renderer->ctx.wgpu_device,
      &(const WGPUBindGroupDescriptor){
          .entryCount = 2,
          .entries =
              (const WGPUBindGroupEntry[]){
                  (WGPUBindGroupEntry){
                      .binding = 0,
                      .textureView =
                          render_graph.resources[deferred_lighting_result.id]
                              .owned_texture.texture_view},
                  (WGPUBindGroupEntry){
                      .binding = 1,
                      .sampler =
                          render_graph.resources[deferred_lighting_result.id]
                              .owned_texture.texture_sampler}},
          .layout = deferred_lighting_result_bind_group_layout});

  CompositionPassData composition_pass_data = {
      .deferred_lighting_result_bind_group =
          deferred_lighting_result_bind_group};
  render_graph_add_pass(
      frame_allocator, &render_graph, &renderer->ctx, &renderer->resources,
      &(const RenderPassDescriptor){
          .identifier = "composition_pass",
          .bind_group_layouts = {deferred_lighting_result_bind_group_layout},
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
          .pass_data = &composition_pass_data,
          .dispatch_fn = composition_pass_dispatch});

  for (size_t light_index = 0; light_index < renderer->resources.light_count;
       light_index++) {
    Light *light = &renderer->resources.lights[light_index];
    if (light->type == LightType_Directional) {
      renderer->resources.common_uniforms.directional_light =
          light->directional_light;

      // computing the light space view proj matrix
      mat4 light_space_projection;
      mat4_set_to_orthographic(light_space_projection, 1.0, 20.0, -20.0, 20.0,
                               14.0, -14.0);

      v3f target = {0};
      // v3f target = light->directional_light.position;
      // v3f_add(&target, &light->directional_light.direction);

      mat4 light_space_view;
      mat4_look_at(light_space_view, &light->directional_light.position,
                   &target, &(const v3f){.y = 1.0});
      mat4_transpose(light_space_view);

      mat4 light_space_projection_from_view_from_local = MAT4_IDENTITY;
      mat4_mul(light_space_projection, light_space_view,
               light_space_projection_from_view_from_local);
      mat4_transpose(light_space_projection_from_view_from_local);
      memcpy(renderer->resources.common_uniforms
                 .light_space_projection_from_view_from_local,
             light_space_projection_from_view_from_local,
             16 * sizeof(mat4_value_type));
    }
  }
  renderer->resources.common_uniforms.current_time_secs = current_time_secs;
  wgpuQueueWriteBuffer(wgpu_queue, renderer->resources.common_uniforms_buffer,
                       0, &renderer->resources.common_uniforms,
                       sizeof(CommonUniforms));

  size_t mesh_uniform_stride = ceilToNextMultiple(
      sizeof(MeshUniforms),
      renderer->ctx.wgpu_limits.limits.minUniformBufferOffsetAlignment);
  for (size_t command_index = 0; command_index < renderer->draw_commands.length;
       command_index++) {
    DrawCommand *command = &renderer->draw_commands.data[command_index];
    MeshUniforms mesh_uniforms = {0};
    transform_matrix(&command->transform,
                     (float *)mesh_uniforms.world_from_local);
    mat4_transpose(mesh_uniforms.world_from_local);
    wgpuQueueWriteBuffer(wgpu_queue, renderer->resources.mesh_uniforms_buffer,
                         command_index * mesh_uniform_stride, &mesh_uniforms,
                         sizeof(MeshUniforms));
  }

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
  wgpuQueueRelease(wgpu_queue);
  wgpuCommandBufferRelease(command_buffer);
  wgpuCommandEncoderRelease(command_encoder);
  wgpuTextureRelease(surface_texture.texture);
  wgpuBindGroupLayoutRelease(deferred_lighting_result_bind_group_layout);
  wgpuBindGroupRelease(deferred_lighting_result_bind_group);
  wgpuBindGroupLayoutRelease(g_buffer_bind_group_layout);
  wgpuBindGroupRelease(g_buffer_bind_group);
  render_graph_deinit(&render_graph);

  for (size_t draw_command_index = 0;
       draw_command_index < renderer->draw_commands.length;
       draw_command_index++) {
    allocator_free(
        renderer->allocator,
        renderer->draw_commands.data[draw_command_index].material_identifier);
  }
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
      render_graph, RenderGraphResourceUsage_DepthStencilAttachment, device,
      WGPUTextureFormat_Depth32Float, width, height);
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
  out_required_limits->limits.maxBufferSize = 248000;
  out_required_limits->limits.maxVertexBufferArrayStride = sizeof(Vertex);
  out_required_limits->limits.maxInterStageShaderComponents = 9;
  out_required_limits->limits.maxTextureDimension2D = 1920;
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
                          .visibility = WGPUShaderStage_Vertex},
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

  renderer->resources.depth_texture = create_depth_texture(
      renderer->ctx.wgpu_device, &renderer->ctx.depth_texture_format);

  DrawCommandQueue_init(renderer->allocator, &renderer->draw_commands);
  cube_mesh_init(renderer->ctx.wgpu_device, queue,
                 &renderer->resources.cube_mesh);

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
  renderer->resources.shader_modules =
      HashTableShaderModule_create(renderer->allocator, 32);
  renderer->resources.textures =
      HashTableTexture_create(renderer->allocator, 512);
  renderer->resources.materials =
      HashTableMaterial_create(renderer->allocator, 32);

  assets_register_loader(assets, Shader, &shader_asset_loader,
                         &shader_asset_destructor);
  assets_register_loader(assets, Material, &material_loader,
                         &material_destructor);
  load_shader_modules(renderer->allocator, renderer->resources.shader_modules,
                      renderer->ctx.wgpu_device, assets);
  load_textures(renderer->allocator, renderer->resources.textures,
                renderer->ctx.wgpu_device, queue, assets);
  load_materials(renderer->allocator, renderer->ctx.wgpu_device,
                 renderer->resources.materials, renderer->resources.textures,
                 renderer->resources.material_bind_group_layout, assets);
}
