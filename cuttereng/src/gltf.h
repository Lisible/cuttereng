#ifndef CUTTERENG_GLTF_H
#define CUTTERENG_GLTF_H

#include "common.h"
#include "math/matrix.h"
#include "math/quaternion.h"
#include "math/vector.h"
#include <lisiblestd/memory.h>

typedef struct {
  char *name;
  size_t node_count;
  size_t *nodes;
} GltfScene;

typedef struct {
  char *name;
  size_t weight_count;
  double *weights;
  size_t children_count;
  size_t *children;
  size_t camera;
  size_t skin;
  size_t mesh;
  mat4 matrix;
  Quaternion rotation;
  v3f scale;
  v3f translation;
  bool has_camera;
  bool has_skin;
  bool has_mesh;
  bool has_matrix;
  bool has_trs;
} GltfNode;

typedef struct {
  char *name;
  size_t accessor;
} GltfMeshPrimitiveAttribute;

typedef struct {
  size_t attribute_count;
  GltfMeshPrimitiveAttribute *attributes;
  size_t indices;
  size_t material;
  size_t mode;
  bool has_indices;
  bool has_material;
  // morph targets
} GltfMeshPrimitive;

GltfMeshPrimitiveAttribute *gltf_mesh_primitive_attribute_by_name(
    const char *name, GltfMeshPrimitiveAttribute *attribute_array,
    size_t attribute_count);

typedef struct {
  char *name;
  size_t weight_count;
  double *weights;
  size_t primitive_count;
  GltfMeshPrimitive *primitives;
} GltfMesh;

typedef enum {
  GltfSamplerMinMagFilter_Nearest = 9728,
  GltfSamplerMinMagFilter_Linear = 9729,
  GltfSamplerMinMagFilter_Unknown
} GltfSamplerMinMagFilter;

typedef enum {
  GltfSamplerWrap_ClampToEdge = 33071,
  GltfSamplerWrap_MirroredRepeat = 33648,
  GltfSamplerWrap_Repeat = 10497,
  GltfSamplerWrap_Unknown
} GltfSamplerWrap;

typedef struct {
  GltfSamplerMinMagFilter mag_filter;
  GltfSamplerMinMagFilter min_filter;
  GltfSamplerWrap wrap_s;
  GltfSamplerWrap wrap_t;
} GltfSampler;

typedef struct {
  char *name;
  size_t buffer_view;
} GltfImage;

typedef struct {
  size_t sampler;
  size_t source;
} GltfTexture;

typedef struct {
  size_t index;
  size_t tex_coord;
} GltfTextureInfo;

typedef struct {
  GltfTextureInfo base_color_texture;
  v4f base_color_factor;
  bool has_base_color_texture;
} GltfMaterialPbrMetallicRoughness;

typedef struct {
  size_t index;
  size_t tex_coord;
  double scale;
} GltfMaterialNormalTextureInfo;

typedef struct {
  char *name;
  GltfMaterialPbrMetallicRoughness pbr_metallic_roughness;
  // GltfMaterialNormalTextureInfo normal_texture;
  bool has_pbr_metallic_roughness;
  // bool has_normal_texture;
} GltfMaterial;

typedef struct {
  char *name;
  char *type;
  const u8 *data_ptr;
  size_t byte_length;
  size_t byte_stride;
  size_t buffer_view_target;
  size_t component_type;
  size_t count;
  double max[16];
  double min[16];
  // GltfAccessorSparse sparse;
  bool normalized;
  bool has_max;
  bool has_min;
  bool has_byte_stride;
  bool has_buffer_view_target;
} GltfAccessor;

typedef struct {
  char *name;
  const u8 *data_ptr;
  size_t buffer;
  size_t byte_offset;
  size_t byte_length;
  size_t byte_stride;
  size_t target;
  bool has_byte_stride;
  bool has_target;
} GltfBufferView;

typedef struct {
  char *name;
  size_t byte_length;
  u8 *data;
} GltfBuffer;

typedef struct {
  int main_scene;
  size_t scene_count;
  GltfScene *scenes;
  size_t node_count;
  GltfNode *nodes;
  size_t mesh_count;
  GltfMesh *meshes;
  size_t material_count;
  GltfMaterial *materials;
  size_t texture_count;
  GltfTexture *textures;
  size_t sampler_count;
  GltfSampler *samplers;
  size_t image_count;
  GltfImage *images;
  size_t accessor_count;
  GltfAccessor *accessors;
  size_t buffer_view_count;
  GltfBufferView *buffer_views;
  // size_t buffer_count;
  // GltfBuffer *buffers;
  const u8 *binary_data;
  bool has_main_scene;
} Gltf;

Gltf *Gltf_parse_glb(Allocator *allocator, const u8 *data,
                     const size_t data_size);
void Gltf_destroy(Allocator *allocator, Gltf *gltf);

#endif // CUTTERENG_GLTF_H
