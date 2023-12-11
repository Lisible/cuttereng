#ifndef CUTTERENG_RENDERER_H
#define CUTTERENG_RENDERER_H

#include "webgpu/webgpu.h"
#include <SDL.h>

typedef struct {
  WGPUInstance wgpu_instance;
  WGPUAdapter wgpu_adapter;
  WGPUSurface wgpu_surface;
  WGPUDevice wgpu_device;
  WGPURenderPipeline pipeline;
  WGPUBuffer vertex_buffer;
  size_t vertex_buffer_length;
} Renderer;

Renderer *renderer_new(SDL_Window *window);
void renderer_destroy(Renderer *renderer);
void renderer_render(Renderer *renderer);

void renderer_initialize_for_window(Renderer *renderer, SDL_Window *window);

#endif // CUTTERENG_RENDERER_H
