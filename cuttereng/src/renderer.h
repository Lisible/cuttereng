#ifndef CUTTERENG_RENDERER_H
#define CUTTERENG_RENDERER_H

#include "wgpu.h"
#include <SDL.h>

typedef struct {
  WGPUInstance wgpu_instance;
  WGPUAdapter wgpu_adapter;
  WGPUSurface wgpu_surface;
  WGPUDevice wgpu_device;
  WGPURenderPipeline pipeline;
} Renderer;

Renderer *renderer_new(SDL_Window *window);
void renderer_destroy(Renderer *renderer);
void renderer_render(Renderer *renderer);

void renderer_initialize_for_window(Renderer *renderer, SDL_Window *window);

#endif // CUTTERENG_RENDERER_H
