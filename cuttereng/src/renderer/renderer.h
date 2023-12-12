#ifndef CUTTERENG_RENDERER_RENDERER_H
#define CUTTERENG_RENDERER_RENDERER_H

#include "../asset.h"
#include <SDL.h>
#include <webgpu/webgpu.h>

typedef struct {
  WGPUInstance wgpu_instance;
  WGPUAdapter wgpu_adapter;
  WGPUSurface wgpu_surface;
  WGPUDevice wgpu_device;
  WGPUTextureFormat wgpu_render_surface_texture_format;
  WGPURenderPipeline pipeline;
  WGPUBuffer vertex_buffer;
  size_t vertex_buffer_length;
} Renderer;

Renderer *renderer_new(SDL_Window *window, Assets *assets);
void renderer_recreate_pipeline(Assets *assets, Renderer *renderer);
void renderer_destroy(Renderer *renderer);
void renderer_render(Renderer *renderer);

void renderer_initialize_for_window(Renderer *renderer, SDL_Window *window);

#endif // CUTTERENG_RENDERER_RENDERER_H
