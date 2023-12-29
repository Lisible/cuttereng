#ifndef CUTTERENG_RENDERER_RENDER_GRAPH_H
#define CUTTERENG_RENDERER_RENDER_GRAPH_H

#include "../memory.h"
#include "renderer.h"
#include <webgpu/webgpu.h>

#define RENDER_GRAPH_MAX_PASS_COUNT 5

typedef struct {
  char *pipeline_identifier;
} RenderPass;

typedef struct {
  RenderPass passes[RENDER_GRAPH_MAX_PASS_COUNT];
  size_t pass_count;
} RenderGraph;

typedef struct {
  char *identifier;
  char *shader_module_identifier;
  bool uses_vertex_buffer;
} RenderPassDescriptor;

void render_graph_init(RenderGraph *render_graph);
void render_graph_add_pass(RenderGraph *render_graph, Assets *assets,
                           RendererContext *ctx, RendererResources *res,
                           const RenderPassDescriptor *render_pass_descriptor);

#endif // CUTTERENG_RENDERER_RENDER_GRAPH_H
