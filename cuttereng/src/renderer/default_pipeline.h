#ifndef CUTTERENG_RENDERER_DEFAULT_PIPELINE_H
#define CUTTERENG_RENDERER_DEFAULT_PIPELINE_H

#include "render_graph.h"
#include "renderer.h"

RendererRenderPipeline *
renderer_default_render_pipeline_create(Allocator *allocator,
                                        Renderer *renderer, Assets *assets);
void renderer_default_render_pipeline_destroy(Allocator *allocator,
                                              RendererRenderPipeline *pipeline);

#endif // CUTTERENG_RENDERER_DEFAULT_PIPELINE_H
