#ifndef CUTTERENG_RENDERER_MATERIAL_H
#define CUTTERENG_RENDERER_MATERIAL_H

#include "../asset.h"
#include "../memory.h"
#include <webgpu/webgpu.h>

typedef struct HashTableTexture HashTableTexture;

typedef enum {
  MaterialType_Basic,
  MaterialType_Shader,
} MaterialType;

typedef struct {
  char *shader_identifier;
} ShaderMaterial;

typedef struct {
  /// Identifier of the base color texture
  char *base_color;
  /// Identifier of the normal map texture
  char *normal;
} Material;

void material_destroy(Allocator *allocator, Material *material);

void *material_loader_fn(Allocator *allocator, const char *path);
extern AssetLoader material_loader;

void material_destructor_fn(Allocator *allocator, void *asset);
extern AssetDestructor material_destructor;

struct GPUMaterial {
  WGPUTextureView base_color_texture_view;
  WGPUSampler base_color_texture_sampler;
  WGPUTextureView normal_texture_view;
  WGPUSampler normal_texture_sampler;
  WGPUBindGroup bind_group;
};

typedef struct GPUMaterial GPUMaterial;

GPUMaterial *gpu_material_create(Allocator *allocator,
                                 HashTableTexture *textures, WGPUDevice device,
                                 WGPUBindGroupLayout material_bind_group_layout,
                                 Material *material);
void gpu_material_destroy(Allocator *allocator, GPUMaterial *material);

#endif // CUTTERENG_RENDERER_MATERIAL_H
