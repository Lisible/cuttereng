#ifndef CUTTERENG_RENDERER_MESH_H
#define CUTTERENG_RENDERER_MESH_H

#include "../asset.h"
#include "../common.h"
#include "../math/vector.h"
#include "../memory.h"
#include <webgpu/webgpu.h>

typedef struct {
  v3 position;
  v3 normal;
  v2 texture_coordinates;
} Vertex;

typedef u16 Index;

typedef struct {
  Vertex *vertices;
  size_t vertex_count;
  Index *indices;
  size_t index_count;
} Mesh;
void mesh_destructor_fn(Allocator *allocator, void *asset);
extern AssetDestructor mesh_destructor;

/// Initializes a mesh with the given vertices and indices
///
/// Note: This function takes ownership of vertices and indices
void mesh_init(Mesh *mesh, Vertex *vertices, size_t vertex_count,
               Index *indices, size_t index_count);
void cube_mesh_init(Mesh *mesh);
void mesh_deinit(Allocator *allocator, Mesh *mesh);
Vertex *mesh_vertices(Mesh *mesh);
size_t mesh_vertex_count(Mesh *mesh);
Index *mesh_indices(Mesh *mesh);
size_t mesh_index_count(Mesh *mesh);

typedef struct {
  Mesh *meshes;
  size_t mesh_count;
} Model;
void *model_asset_loader_fn(Allocator *allocator, Assets *assets,
                            const char *path);
extern AssetLoader model_asset_loader;
void model_asset_destructor_fn(Allocator *allocator, void *asset);
extern AssetDestructor model_asset_destructor;

typedef struct {
  WGPUBuffer vertex_buffer;
  WGPUBuffer index_buffer;
  size_t vertex_count;
  size_t index_count;
} GPUMesh;

bool gpu_mesh_init(WGPUDevice device, WGPUQueue queue, GPUMesh *gpu_mesh,
                   Mesh *mesh);
void gpu_mesh_bind(WGPURenderPassEncoder rpe, GPUMesh *gpu_mesh);
void gpu_mesh_deinit(GPUMesh *gpu_mesh);

#endif // CUTTERENG_RENDERER_MESH_H
