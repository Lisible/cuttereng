#include "mesh.h"
#include "../assert.h"
#include "webgpu/webgpu.h"

void mesh_init(Mesh *mesh, Vertex *vertices, size_t vertex_count,
               Index *indices, size_t index_count) {
  ASSERT(mesh != NULL);
  ASSERT(vertices != NULL);
  ASSERT(indices != NULL);
  mesh->vertices = vertices;
  mesh->vertex_count = vertex_count;
  mesh->indices = indices;
  mesh->index_count = index_count;
}
void mesh_deinit(Allocator *allocator, Mesh *mesh) {
  ASSERT(mesh != NULL);
  allocator_free(allocator, mesh->vertices);
  allocator_free(allocator, mesh->indices);
}
Vertex *mesh_vertices(Mesh *mesh) {
  ASSERT(mesh != NULL);
  return mesh->vertices;
}
size_t mesh_vertex_count(Mesh *mesh) {
  ASSERT(mesh != NULL);
  return mesh->vertex_count;
}
Index *mesh_indices(Mesh *mesh) {
  ASSERT(mesh != NULL);
  return mesh->indices;
}
size_t mesh_index_count(Mesh *mesh) {
  ASSERT(mesh != NULL);
  return mesh->index_count;
}

bool gpu_mesh_init(WGPUDevice device, WGPUQueue queue, GPUMesh *gpu_mesh,
                   Mesh *mesh) {
  ASSERT(device != NULL);
  ASSERT(queue != NULL);
  ASSERT(mesh != NULL);
  ASSERT(gpu_mesh != NULL);

  gpu_mesh->vertex_buffer = wgpuDeviceCreateBuffer(
      device, &(const WGPUBufferDescriptor){
                  .usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
                  .size = mesh->vertex_count * sizeof(Vertex)});
  if (!gpu_mesh->vertex_buffer) {
    goto err;
  }
  wgpuQueueWriteBuffer(queue, gpu_mesh->vertex_buffer, 0, mesh->vertices,
                       mesh->vertex_count * sizeof(Vertex));

  gpu_mesh->vertex_count = mesh->vertex_count;

  // NOTE: The writeBuffer operation needs to be 4-bytes aligned.
  size_t index_buffer_size = (mesh->index_count * sizeof(Index) + 3) & ~3;
  gpu_mesh->index_buffer = wgpuDeviceCreateBuffer(
      device, &(const WGPUBufferDescriptor){.usage = WGPUBufferUsage_Index |
                                                     WGPUBufferUsage_CopyDst,
                                            .size = index_buffer_size});
  if (!gpu_mesh->index_buffer) {
    goto cleanup_vertex_buffer;
  }
  wgpuQueueWriteBuffer(queue, gpu_mesh->index_buffer, 0, mesh->indices,
                       index_buffer_size);

  gpu_mesh->index_count = mesh->index_count;

  return true;

cleanup_vertex_buffer:
  wgpuBufferDestroy(gpu_mesh->vertex_buffer);
  wgpuBufferRelease(gpu_mesh->vertex_buffer);
err:
  return false;
}

void gpu_mesh_deinit(GPUMesh *gpu_mesh) {
  ASSERT(gpu_mesh != NULL);
  wgpuBufferDestroy(gpu_mesh->vertex_buffer);
  wgpuBufferRelease(gpu_mesh->vertex_buffer);
  wgpuBufferDestroy(gpu_mesh->index_buffer);
  wgpuBufferRelease(gpu_mesh->index_buffer);
}

void gpu_mesh_bind(WGPURenderPassEncoder rpe, GPUMesh *gpu_mesh) {
  ASSERT(gpu_mesh != NULL);
  wgpuRenderPassEncoderSetVertexBuffer(rpe, 0, gpu_mesh->vertex_buffer, 0,
                                       gpu_mesh->vertex_count * sizeof(Vertex));
  wgpuRenderPassEncoderSetIndexBuffer(rpe, gpu_mesh->index_buffer,
                                      WGPUIndexFormat_Uint16, 0,
                                      gpu_mesh->index_count * sizeof(Index));
}
