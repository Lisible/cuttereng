#include "mesh.h"
#include "../assert.h"
#include "../gltf.h"
#include "src/asset.h"
#include "src/bytes.h"
#include "webgpu/webgpu.h"
#include <string.h>

static Vertex CUBE_VERTICES[36] = {(Vertex){.position = {-0.5, 0.5, -0.5},
                                            .texture_coordinates = {0.0, 1.0},
                                            .normal = {0.0, 0.0, -1.0}},
                                   (Vertex){.position = {-0.5, -0.5, -0.5},
                                            .texture_coordinates = {0.0, 0.0},
                                            .normal = {0.0, 0.0, -1.0}},
                                   (Vertex){.position = {0.5, -0.5, -0.5},
                                            .texture_coordinates = {1.0, 0.0},
                                            .normal = {0.0, 0.0, -1.0}},
                                   (Vertex){.position = {0.5, -0.5, -0.5},
                                            .texture_coordinates = {1.0, 0.0},
                                            .normal = {0.0, 0.0, -1.0}},
                                   (Vertex){.position = {0.5, 0.5, -0.5},
                                            .texture_coordinates = {1.0, 1.0},
                                            .normal = {0.0, 0.0, -1.0}},
                                   (Vertex){.position = {-0.5, 0.5, -0.5},
                                            .texture_coordinates = {0.0, 1.0},
                                            .normal = {0.0, 0.0, -1.0}},
                                   // Right face
                                   (Vertex){.position = {0.5, 0.5, -0.5},
                                            .texture_coordinates = {0.0, 1.0},
                                            .normal = {1.0, 0.0, 0.0}},
                                   (Vertex){.position = {0.5, -0.5, -0.5},
                                            .texture_coordinates = {0.0, 0.0},
                                            .normal = {1.0, 0.0, 0.0}},
                                   (Vertex){.position = {0.5, -0.5, 0.5},
                                            .texture_coordinates = {1.0, 0.0},
                                            .normal = {1.0, 0.0, 0.0}},
                                   (Vertex){.position = {0.5, -0.5, 0.5},
                                            .texture_coordinates = {1.0, 0.0},
                                            .normal = {1.0, 0.0, 0.0}},
                                   (Vertex){.position = {0.5, 0.5, 0.5},
                                            .texture_coordinates = {1.0, 1.0},
                                            .normal = {1.0, 0.0, 0.0}},
                                   (Vertex){.position = {0.5, 0.5, -0.5},
                                            .texture_coordinates = {0.0, 1.0},
                                            .normal = {1.0, 0.0, 0.0}},
                                   // Front face
                                   (Vertex){.position = {-0.5, 0.5, 0.5},
                                            .texture_coordinates = {1.0, 1.0},
                                            .normal = {0.0, 0.0, 1.0}},
                                   (Vertex){.position = {0.5, -0.5, 0.5},
                                            .texture_coordinates = {0.0, 0.0},
                                            .normal = {0.0, 0.0, 1.0}},
                                   (Vertex){.position = {-0.5, -0.5, 0.5},
                                            .texture_coordinates = {1.0, 0.0},
                                            .normal = {0.0, 0.0, 1.0}},
                                   (Vertex){.position = {0.5, 0.5, 0.5},
                                            .texture_coordinates = {0.0, 1.0},
                                            .normal = {0.0, 0.0, 1.0}},
                                   (Vertex){.position = {0.5, -0.5, 0.5},
                                            .texture_coordinates = {0.0, 0.0},
                                            .normal = {0.0, 0.0, 1.0}},
                                   (Vertex){.position = {-0.5, 0.5, 0.5},
                                            .texture_coordinates = {1.0, 1.0},
                                            .normal = {0.0, 0.0, 1.0}},
                                   // Left face
                                   (Vertex){.position = {-0.5, 0.5, 0.5},
                                            .texture_coordinates = {0.0, 1.0},
                                            .normal = {-1.0, 0.0, 0.0}},
                                   (Vertex){.position = {-0.5, -0.5, 0.5},
                                            .texture_coordinates = {0.0, 0.0},
                                            .normal = {-1.0, 0.0, 0.0}},
                                   (Vertex){.position = {-0.5, -0.5, -0.5},
                                            .texture_coordinates = {1.0, 0.0},
                                            .normal = {-1.0, 0.0, 0.0}},
                                   (Vertex){.position = {-0.5, -0.5, -0.5},
                                            .texture_coordinates = {1.0, 0.0},
                                            .normal = {-1.0, 0.0, 0.0}},
                                   (Vertex){.position = {-0.5, 0.5, -0.5},
                                            .texture_coordinates = {1.0, 1.0},
                                            .normal = {-1.0, 0.0, 0.0}},
                                   (Vertex){.position = {-0.5, 0.5, 0.5},
                                            .texture_coordinates = {0.0, 1.0},
                                            .normal = {-1.0, 0.0, 0.0}},
                                   // Bottom face
                                   (Vertex){.position = {-0.5, -0.5, -0.5},
                                            .texture_coordinates = {0.0, 0.0},
                                            .normal = {0.0, -1.0, 0.0}},
                                   (Vertex){.position = {-0.5, -0.5, 0.5},
                                            .texture_coordinates = {0.0, 1.0},
                                            .normal = {0.0, -1.0, 0.0}},
                                   (Vertex){.position = {0.5, -0.5, -0.5},
                                            .texture_coordinates = {1.0, 0.0},
                                            .normal = {0.0, -1.0, 0.0}},
                                   (Vertex){.position = {0.5, -0.5, 0.5},
                                            .texture_coordinates = {1.0, 1.0},
                                            .normal = {0.0, -1.0, 0.0}},
                                   (Vertex){.position = {0.5, -0.5, -0.5},
                                            .texture_coordinates = {1.0, 0.0},
                                            .normal = {0.0, -1.0, 0.0}},
                                   (Vertex){.position = {-0.5, -0.5, 0.5},
                                            .texture_coordinates = {0.0, 1.0},
                                            .normal = {0.0, -1.0, 0.0}},
                                   // Top face
                                   (Vertex){.position = {-0.5, 0.5, 0.5},
                                            .texture_coordinates = {0.0, 1.0},
                                            .normal = {0.0, 1.0, 0.0}},
                                   (Vertex){.position = {-0.5, 0.5, -0.5},
                                            .texture_coordinates = {0.0, 0.0},
                                            .normal = {0.0, 1.0, 0.0}},
                                   (Vertex){.position = {0.5, 0.5, -0.5},
                                            .texture_coordinates = {1.0, 0.0},
                                            .normal = {0.0, 1.0, 0.0}},
                                   (Vertex){.position = {0.5, 0.5, -0.5},
                                            .texture_coordinates = {1.0, 0.0},
                                            .normal = {0.0, 1.0, 0.0}},
                                   (Vertex){.position = {0.5, 0.5, 0.5},
                                            .texture_coordinates = {1.0, 1.0},
                                            .normal = {0.0, 1.0, 0.0}},
                                   (Vertex){.position = {-0.5, 0.5, 0.5},
                                            .texture_coordinates = {0.0, 1.0},
                                            .normal = {0.0, 1.0, 0.0}}};

void cube_model_init(Allocator *allocator, WGPUDevice device, WGPUQueue queue,
                     GPUModel *model) {
  Mesh cube_mesh = {.vertices = CUBE_VERTICES, .vertex_count = 36};
  Model cube_model = {.mesh_count = 1, .meshes = &cube_mesh};
  GPUModel_init(allocator, device, queue, model, &cube_model);
}

void mesh_init(Mesh *mesh, Vertex *vertices, size_t vertex_count,
               Index *indices, size_t index_count) {
  ASSERT(mesh != NULL);
  ASSERT(vertices != NULL);
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
  if (mesh->index_count > 0) {
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
  } else {
    gpu_mesh->index_buffer = NULL;
  }

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

  if (gpu_mesh->index_buffer != NULL) {
    wgpuBufferDestroy(gpu_mesh->index_buffer);
    wgpuBufferRelease(gpu_mesh->index_buffer);
  }
}

void gpu_mesh_bind(WGPURenderPassEncoder rpe, GPUMesh *gpu_mesh) {
  ASSERT(gpu_mesh != NULL);
  wgpuRenderPassEncoderSetVertexBuffer(rpe, 0, gpu_mesh->vertex_buffer, 0,
                                       gpu_mesh->vertex_count * sizeof(Vertex));

  if (gpu_mesh->index_count > 0) {
    wgpuRenderPassEncoderSetIndexBuffer(rpe, gpu_mesh->index_buffer,
                                        WGPUIndexFormat_Uint16, 0,
                                        gpu_mesh->index_count * sizeof(Index));
  }
}
void GPUModel_init(Allocator *allocator, WGPUDevice device, WGPUQueue queue,
                   GPUModel *gpu_model, const Model *model) {
  ASSERT(device != NULL);
  ASSERT(queue != NULL);
  ASSERT(gpu_model != NULL);
  ASSERT(model != NULL);

  gpu_model->mesh_count = model->mesh_count;
  gpu_model->meshes =
      allocator_allocate_array(allocator, model->mesh_count, sizeof(GPUMesh));
  for (size_t mesh_index = 0; mesh_index < model->mesh_count; mesh_index++) {
    gpu_mesh_init(device, queue, &gpu_model->meshes[mesh_index],
                  &model->meshes[mesh_index]);
  }
}
void GPUModel_deinit(Allocator *allocator, GPUModel *gpu_model) {
  ASSERT(gpu_model != NULL);
  for (size_t mesh_index = 0; mesh_index < gpu_model->mesh_count;
       mesh_index++) {
    gpu_mesh_deinit(&gpu_model->meshes[mesh_index]);
  }
  allocator_free(allocator, gpu_model->meshes);
}

void mesh_destructor_fn(Allocator *allocator, void *asset) {
  ASSERT(allocator != NULL);
  ASSERT(asset != NULL);
  Mesh *mesh = asset;
  mesh_deinit(allocator, mesh);
  allocator_free(allocator, mesh);
}
AssetDestructor mesh_destructor = {.fn = mesh_destructor_fn};

void *model_asset_loader_fn(Allocator *allocator, const char *path) {
  ASSERT(allocator != NULL);
  ASSERT(path != NULL);
  // FIXME This is not error checked and this is platform specific
  // There should be an abstraction that allows to do that
  // The abstraction should also provide a way to not have to put all the
  // content in a big buffer
  char *effective_path = asset_get_effective_path(allocator, path);
  LOG_DEBUG("effective_path: %s", effective_path);
  FILE *glb_file = fopen(effective_path, "r");
  fseek(glb_file, 0, SEEK_END);
  long glb_file_length = ftell(glb_file);
  fseek(glb_file, 0, SEEK_SET);

  u8 *content = allocator_allocate(allocator, glb_file_length);
  fread(content, 1, glb_file_length, glb_file);
  fclose(glb_file);
  allocator_free(allocator, effective_path);

  Gltf *gltf = Gltf_parse_glb(allocator, content, glb_file_length);
  size_t mesh_count = gltf->mesh_count;
  Model *model = allocator_allocate(allocator, sizeof(Model));
  model->mesh_count = mesh_count;
  model->meshes = allocator_allocate_array(allocator, mesh_count, sizeof(Mesh));
  for (size_t mesh_index = 0; mesh_index < mesh_count; mesh_index++) {
    Mesh *mesh = &model->meshes[mesh_index];
    GltfMesh *gltf_mesh = &gltf->meshes[mesh_index];
    for (size_t primitive_index = 0;
         primitive_index < gltf_mesh->primitive_count; primitive_index++) {
      GltfMeshPrimitive *gltf_mesh_primitive =
          &gltf_mesh->primitives[primitive_index];
      LOG_DEBUG("has indices: %b", gltf_mesh_primitive->has_indices);
      GltfMeshPrimitiveAttribute *position_attribute =
          gltf_mesh_primitive_attribute_by_name(
              "POSITION", gltf_mesh_primitive->attributes,
              gltf_mesh_primitive->attribute_count);
      GltfAccessor *position_accessor =
          &gltf->accessors[position_attribute->accessor];
      GltfMeshPrimitiveAttribute *normal_attribute =
          gltf_mesh_primitive_attribute_by_name(
              "NORMAL", gltf_mesh_primitive->attributes,
              gltf_mesh_primitive->attribute_count);
      GltfAccessor *normal_accessor =
          normal_attribute == NULL
              ? NULL
              : &gltf->accessors[normal_attribute->accessor];
      GltfMeshPrimitiveAttribute *texture_coordinates_attribute =
          gltf_mesh_primitive_attribute_by_name(
              "TEXCOORD_0", gltf_mesh_primitive->attributes,
              gltf_mesh_primitive->attribute_count);
      GltfAccessor *texture_coordinates_accessor =
          texture_coordinates_attribute == NULL
              ? NULL
              : &gltf->accessors[texture_coordinates_attribute->accessor];

      mesh->vertex_count = position_accessor->count;
      mesh->vertices = allocator_allocate_array(
          allocator, position_accessor->count, sizeof(Vertex));
      for (size_t vertex_index = 0; vertex_index < position_accessor->count;
           vertex_index++) {
        size_t position_stride = position_accessor->byte_stride;
        if (position_stride == 0) {
          position_stride = sizeof(v3f);
        }

        mesh->vertices[vertex_index].position.x = float_from_bytes_le(
            &position_accessor->data_ptr[vertex_index * position_stride]);
        mesh->vertices[vertex_index].position.y = float_from_bytes_le(
            &position_accessor
                 ->data_ptr[vertex_index * position_stride + sizeof(float)]);
        mesh->vertices[vertex_index].position.z = -float_from_bytes_le(
            &position_accessor->data_ptr[vertex_index * position_stride +
                                         2 * sizeof(float)]);

        if (normal_accessor != NULL) {
          size_t normal_stride = normal_accessor->byte_stride;
          if (normal_stride == 0) {
            normal_stride = sizeof(v3f);
          }
          mesh->vertices[vertex_index].normal.x = float_from_bytes_le(
              &normal_accessor->data_ptr[vertex_index * normal_stride]);
          mesh->vertices[vertex_index].normal.y = float_from_bytes_le(
              &normal_accessor
                   ->data_ptr[vertex_index * normal_stride + sizeof(float)]);
          mesh->vertices[vertex_index].normal.z = float_from_bytes_le(
              &normal_accessor->data_ptr[vertex_index * normal_stride +
                                         2 * sizeof(float)]);
        }
        if (texture_coordinates_accessor != NULL) {
          size_t texture_coordinates_stride =
              texture_coordinates_accessor->byte_stride;
          if (texture_coordinates_stride == 0) {
            texture_coordinates_stride = sizeof(v2f);
          }
          mesh->vertices[vertex_index].texture_coordinates.x =
              float_from_bytes_le(
                  &texture_coordinates_accessor
                       ->data_ptr[vertex_index * texture_coordinates_stride]);
          mesh->vertices[vertex_index].texture_coordinates.y =
              float_from_bytes_le(
                  &texture_coordinates_accessor
                       ->data_ptr[vertex_index * texture_coordinates_stride +
                                  sizeof(float)]);
        }
      }
      if (gltf_mesh_primitive->has_indices) {
        GltfAccessor *index_accessor =
            &gltf->accessors[gltf_mesh_primitive->indices];
        size_t index_count = index_accessor->count;
        mesh->index_count = index_count;
        mesh->indices =
            allocator_allocate_array(allocator, index_count, sizeof(Index));
        for (size_t index_index = 0; index_index < index_count; index_index++) {
          size_t stride = index_accessor->byte_stride;
          if (stride == 0) {
            stride = sizeof(Index);
          }
          mesh->indices[index_index] = u16_from_bytes_le(
              &index_accessor->data_ptr[index_index * stride]);
        }
      }
    }
  }

  return model;
}
AssetLoader model_asset_loader = {.fn = model_asset_loader_fn};
void model_asset_destructor_fn(Allocator *allocator, void *asset) {}
AssetDestructor model_asset_destructor = {.fn = model_asset_destructor_fn};
