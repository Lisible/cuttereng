#include "gltf.h"
#include "assert.h"
#include "bytes.h"
#include "src/json.h"
#include "src/memory.h"
#include <string.h>

#define GLB_MAGIC_NUMBER 0x46546C67

#define GLB_CHUNK_TYPE_JSON 0x4E4F534A
#define GLB_CHUNK_TYPE_BIN 0x004E4942

// FIXME Add allocation error handling

typedef struct {
  Allocator *allocator;
  const u8 *data;
  size_t data_size;
  size_t current_index;
} ParsingContext;
void ParsingContext_init(ParsingContext *ctx, Allocator *allocator,
                         const u8 *data, const size_t data_size) {
  ctx->allocator = allocator;
  ctx->current_index = 0;
  ctx->data = data;
  ctx->data_size = data_size;
}
u32 ParsingContext_parse_u32(ParsingContext *ctx) {
  size_t advance = sizeof(u32);
  ASSERT(ctx->current_index + advance < ctx->data_size);
  u32 value = u32_from_bytes_le(&ctx->data[ctx->current_index]);
  ctx->current_index += advance;
  return value;
}
void ParsingContext_skip_bytes(ParsingContext *ctx, size_t byte_count) {
  ASSERT(ctx->current_index + byte_count <= ctx->data_size);
  ctx->current_index += byte_count;
}

void parse_main_scene(const ParsingContext *ctx, Gltf *gltf,
                      const JsonObject *gltf_json);
bool parse_scenes(const ParsingContext *ctx, Gltf *gltf,
                  const JsonArray *gltf_scenes_json);
bool GltfScene_parse(const ParsingContext *ctx, GltfScene *gltf_scene,
                     const JsonObject *gltf_scene_json);
void GltfScene_deinit(Allocator *allocator, GltfScene *gltf_scene);
bool parse_nodes(const ParsingContext *ctx, Gltf *gltf,
                 const JsonArray *gltf_nodes_json);
bool GltfNode_parse(const ParsingContext *ctx, GltfNode *gltf_node,
                    const JsonObject *gltf_node_json);
void GltfNode_deinit(Allocator *allocator, GltfNode *gltf_node);
bool parse_meshes(const ParsingContext *ctx, Gltf *gltf,
                  const JsonArray *gltf_meshes_json);
bool GltfMesh_parse(const ParsingContext *ctx, GltfMesh *gltf_mesh,
                    const JsonObject *gltf_mesh_json);
void GltfMesh_deinit(Allocator *allocator, GltfMesh *gltf_mesh);
bool GltfMeshPrimitive_parse(const ParsingContext *ctx,
                             GltfMeshPrimitive *gltf_mesh,
                             const JsonObject *gltf_mesh_primitive_json);
void GltfMeshPrimitive_deinit(Allocator *allocator,
                              GltfMeshPrimitive *gltf_mesh_primitive);
bool parse_accessors(const ParsingContext *ctx, Gltf *gltf, size_t buffer_size,
                     const u8 *buffer, size_t buffer_view_count,
                     GltfBufferView *buffer_views,
                     const JsonArray *gltf_accessors_json);
bool GltfAccessor_parse(const ParsingContext *ctx, GltfAccessor *gltf_accessor,
                        size_t buffer_size, const u8 *buffer,
                        size_t buffer_view_count, GltfBufferView *buffer_views,
                        const JsonObject *gltf_accessor_json);
void GltfAccessor_deinit(Allocator *allocator, GltfAccessor *gltf_accessor);
bool parse_buffer_views(const ParsingContext *ctx,
                        size_t *out_buffer_view_count,
                        GltfBufferView **out_buffer_views,
                        const JsonArray *gltf_buffer_views_json);
bool GltfBufferView_parse(const ParsingContext *ctx,
                          GltfBufferView *gltf_buffer_view,
                          const JsonObject *gltf_buffer_view_json);
void GltfBufferView_deinit(Allocator *allocator,
                           GltfBufferView *gltf_buffer_view);

Gltf *Gltf_parse_glb(Allocator *allocator, const u8 *data,
                     const size_t data_size) {
  ASSERT(data != NULL);
  ParsingContext ctx;
  ParsingContext_init(&ctx, allocator, data, data_size);

  u32 magic_number = ParsingContext_parse_u32(&ctx);
  LOG_DEBUG("GLB magic number: %x", magic_number);
  if (magic_number != GLB_MAGIC_NUMBER) {
    LOG_ERROR("Invalid GLB magic number");
    goto err;
  }

  u32 version = ParsingContext_parse_u32(&ctx);
  LOG_DEBUG("%x", version);
  if (version != 2) {
    LOG_ERROR("Only GLB version 2 is supported");
    goto err;
  }
  u32 length = ParsingContext_parse_u32(&ctx);
  LOG_DEBUG("GLB length: %d", length);

  u32 json_chunk_data_length = ParsingContext_parse_u32(&ctx);
  LOG_DEBUG("GLB json chunk data length: %d", json_chunk_data_length);
  u32 json_chunk_type = ParsingContext_parse_u32(&ctx);
  if (json_chunk_type != GLB_CHUNK_TYPE_JSON) {
    LOG_ERROR("GLB JSON chunk has wrong type");
    goto err;
  }
  const u8 *json_chunk_data = &ctx.data[ctx.current_index];
  ParsingContext_skip_bytes(&ctx, json_chunk_data_length);
  Json *gltf_json = json_parse_from_str(allocator, (char *)json_chunk_data);
  if (gltf_json == NULL) {
    LOG_ERROR("Couldn't parse GLTF json");
    goto err;
  }

  JsonObject *gltf_json_object = json_as_object(gltf_json);
  if (gltf_json_object == NULL) {
    LOG_ERROR("The root value of GLTF json is not an object");
    goto err;
  }

  u32 binary_chunk_data_length = ParsingContext_parse_u32(&ctx);
  LOG_DEBUG("GLB binary chunk data length: %d", binary_chunk_data_length);
  u32 binary_chunk_type = ParsingContext_parse_u32(&ctx);
  if (binary_chunk_type != GLB_CHUNK_TYPE_BIN) {
    LOG_ERROR("GLB binary chunk has wrong type");
    goto err;
  }
  const u8 *binary_chunk_data = &ctx.data[ctx.current_index];
  ParsingContext_skip_bytes(&ctx, binary_chunk_data_length);

  Gltf *gltf = allocator_allocate(allocator, sizeof(Gltf));
  parse_main_scene(&ctx, gltf, gltf_json_object);

  JsonArray *gltf_scenes_json = NULL;
  json_object_get_array(gltf_json_object, "scenes", &gltf_scenes_json);
  parse_scenes(&ctx, gltf, gltf_scenes_json);

  JsonArray *gltf_nodes_json = NULL;
  json_object_get_array(gltf_json_object, "nodes", &gltf_nodes_json);
  parse_nodes(&ctx, gltf, gltf_nodes_json);

  JsonArray *gltf_meshes_json = NULL;
  json_object_get_array(gltf_json_object, "meshes", &gltf_meshes_json);
  parse_meshes(&ctx, gltf, gltf_meshes_json);

  JsonArray *gltf_buffer_views_json = NULL;
  json_object_get_array(gltf_json_object, "bufferViews",
                        &gltf_buffer_views_json);

  size_t buffer_view_count = 0;
  GltfBufferView *buffer_views = NULL;
  parse_buffer_views(&ctx, &buffer_view_count, &buffer_views,
                     gltf_buffer_views_json);

  JsonArray *gltf_accessors_json = NULL;
  json_object_get_array(gltf_json_object, "accessors", &gltf_accessors_json);
  parse_accessors(&ctx, gltf, binary_chunk_data_length, binary_chunk_data,
                  buffer_view_count, buffer_views, gltf_accessors_json);

  // cleaning up buffer views
  for (size_t i = 0; i < buffer_view_count; i++) {
    allocator_free(ctx.allocator, buffer_views[i].name);
  }
  allocator_free(ctx.allocator, buffer_views);

  gltf->binary_data = binary_chunk_data;

  json_destroy(allocator, gltf_json);
  return gltf;

cleanup_gltf_json:
  allocator_free(allocator, gltf_json);
  allocator_free(allocator, gltf);
err:
  return NULL;
}

void Gltf_destroy(Allocator *allocator, Gltf *gltf) {
  for (size_t accessor_index = 0; accessor_index < gltf->accessor_count;
       accessor_index++) {
    GltfAccessor_deinit(allocator, &gltf->accessors[accessor_index]);
  }
  allocator_free(allocator, gltf->accessors);

  for (size_t mesh_index = 0; mesh_index < gltf->mesh_count; mesh_index++) {
    GltfMesh_deinit(allocator, &gltf->meshes[mesh_index]);
  }
  allocator_free(allocator, gltf->meshes);

  for (size_t node_index = 0; node_index < gltf->node_count; node_index++) {
    GltfNode_deinit(allocator, &gltf->nodes[node_index]);
  }
  allocator_free(allocator, gltf->nodes);

  for (size_t scene_index = 0; scene_index < gltf->scene_count; scene_index++) {
    GltfScene_deinit(allocator, &gltf->scenes[scene_index]);
  }
  allocator_free(allocator, gltf->scenes);

  allocator_free(allocator, gltf);
}

void parse_main_scene(const ParsingContext *ctx, Gltf *gltf,
                      const JsonObject *gltf_json) {
  ASSERT(ctx != NULL);
  ASSERT(gltf != NULL);
  ASSERT(gltf_json != NULL);
  double main_scene;
  if (!json_object_get_number(gltf_json, "mainScene", &main_scene)) {
    LOG_DEBUG("no main scene found", gltf->main_scene);
    gltf->has_main_scene = false;
  } else {
    gltf->main_scene = (int)main_scene;
    LOG_DEBUG("main scene is %d", gltf->main_scene);
    gltf->has_main_scene = true;
  }
}

bool parse_scenes(const ParsingContext *ctx, Gltf *gltf,
                  const JsonArray *gltf_scenes_json) {
  ASSERT(ctx != NULL);
  ASSERT(gltf != NULL);
  ASSERT(gltf_scenes_json != NULL);
  LOG_DEBUG("Parsing scenes");
  size_t scene_count = json_array_length(gltf_scenes_json);
  LOG_DEBUG("Scene count: %d", scene_count);
  gltf->scene_count = scene_count;
  gltf->scenes =
      allocator_allocate_array(ctx->allocator, scene_count, sizeof(GltfScene));

  for (size_t scene_index = 0; scene_index < scene_count; scene_index++) {
    Json *scene_json = json_array_at(gltf_scenes_json, scene_index);
    if (scene_json->type != JSON_OBJECT) {
      return false;
    }

    LOG_DEBUG("Parsing scene %d", scene_index);
    if (!GltfScene_parse(ctx, &gltf->scenes[scene_index], scene_json->object)) {
      return false;
    }
  }

  return true;
}

bool GltfScene_parse(const ParsingContext *ctx, GltfScene *gltf_scene,
                     const JsonObject *gltf_scene_json) {
  ASSERT(ctx != NULL);
  ASSERT(gltf_scene != NULL);
  ASSERT(gltf_scene_json != NULL);

  char *scene_name = NULL;
  json_object_get_string(gltf_scene_json, "name", &scene_name);
  if (scene_name != NULL) {
    scene_name = memory_clone_string(ctx->allocator, scene_name);
    LOG_DEBUG("Scene name: %s", scene_name);
  }
  gltf_scene->name = scene_name;

  JsonArray *nodes_array = NULL;
  json_object_get_array(gltf_scene_json, "nodes", &nodes_array);

  if (nodes_array != NULL) {
    size_t node_count = json_array_length(nodes_array);
    gltf_scene->node_count = node_count;
    gltf_scene->nodes =
        allocator_allocate_array(ctx->allocator, node_count, sizeof(size_t));
    for (size_t node_index = 0; node_index < node_count; node_index++) {
      Json *node_json = json_array_at(nodes_array, node_index);
      if (node_json->type != JSON_NUMBER) {
        return false;
      }

      gltf_scene->nodes[node_index] = (size_t)node_json->number;
      LOG_DEBUG("Node %d = %d", node_index, gltf_scene->nodes[node_index]);
    }
  }

  return true;
}

bool parse_nodes(const ParsingContext *ctx, Gltf *gltf,
                 const JsonArray *gltf_nodes_json) {
  ASSERT(ctx != NULL);
  ASSERT(gltf != NULL);
  ASSERT(gltf_nodes_json != NULL);
  LOG_DEBUG("Parsing nodes");

  size_t node_count = json_array_length(gltf_nodes_json);
  gltf->node_count = node_count;
  gltf->nodes =
      allocator_allocate_array(ctx->allocator, node_count, sizeof(GltfNode));

  for (size_t node_index = 0; node_index < node_count; node_index++) {
    LOG_DEBUG("Parsing node %d", node_index);
    Json *node_json = json_array_at(gltf_nodes_json, node_index);
    if (node_json->type != JSON_OBJECT) {
      LOG_DEBUG("Node type is not object");
      return false;
    }

    if (!GltfNode_parse(ctx, &gltf->nodes[node_index], node_json->object)) {
      LOG_DEBUG("Couldn't parse node");
      return false;
    }
  }

  return true;
}

bool GltfNode_parse(const ParsingContext *ctx, GltfNode *gltf_node,
                    const JsonObject *gltf_node_json) {
  ASSERT(ctx != NULL);
  ASSERT(gltf_node != NULL);
  ASSERT(gltf_node_json != NULL);

  char *node_name = NULL;
  json_object_get_string(gltf_node_json, "name", &node_name);
  if (node_name != NULL) {
    node_name = memory_clone_string(ctx->allocator, node_name);
    LOG_DEBUG("Node name: %s", node_name);
  }
  gltf_node->name = node_name;
  LOG_DEBUG("Parsing node %s", gltf_node->name);

  double camera;
  if (!json_object_get_number(gltf_node_json, "camera", &camera)) {
    gltf_node->has_camera = false;
    LOG_DEBUG("Node has no camera");
  } else {
    gltf_node->camera = (size_t)camera;
    gltf_node->has_camera = true;
    LOG_DEBUG("Node camera: %d", gltf_node->camera);
  }

  JsonArray *children = NULL;
  if (!json_object_get_array(gltf_node_json, "children", &children)) {
    gltf_node->children_count = 0;
    LOG_DEBUG("Node has no child");
  } else {
    size_t children_count = json_array_length(children);
    gltf_node->children_count = children_count;
    gltf_node->children = allocator_allocate_array(
        ctx->allocator, children_count, sizeof(size_t));
    for (size_t child_index = 0; child_index < children_count; child_index++) {
      Json *child = json_array_at(children, child_index);
      if (child->type != JSON_NUMBER) {
        return false;
      }

      gltf_node->children[child_index] = (size_t)child->number;
      LOG_DEBUG("node->children[%d]: %d", child_index,
                gltf_node->children[child_index]);
    }
  }

  double skin;
  if (!json_object_get_number(gltf_node_json, "skin", &skin)) {
    gltf_node->has_skin = false;
    LOG_DEBUG("Node has no skin");
  } else {
    gltf_node->skin = (size_t)skin;
    gltf_node->has_skin = true;
    LOG_DEBUG("Node has skin");
  }

  JsonArray *matrix = NULL;
  if (!json_object_get_array(gltf_node_json, "matrix", &matrix)) {
    gltf_node->has_matrix = false;
    memset(gltf_node->matrix, 0, 16);
    gltf_node->matrix[0] = 1.0;
    gltf_node->matrix[5] = 1.0;
    gltf_node->matrix[10] = 1.0;
    gltf_node->matrix[15] = 1.0;
  } else {
    gltf_node->has_matrix = true;
    size_t matrix_length = json_array_length(matrix);
    for (size_t i = 0; i < matrix_length; i++) {
      Json *value = json_array_at(matrix, i);
      if (value->type != JSON_NUMBER) {
        return false;
      }

      gltf_node->matrix[i] = value->number;
    }
  }

  double mesh;
  if (!json_object_get_number(gltf_node_json, "mesh", &mesh)) {
    LOG_DEBUG("Node has no mesh");
    gltf_node->has_mesh = false;
  } else {
    gltf_node->mesh = (size_t)mesh;
    gltf_node->has_mesh = true;
  }

  gltf_node->has_trs = false;
  JsonArray *rotation = NULL;
  if (!json_object_get_array(gltf_node_json, "rotation", &rotation)) {
    gltf_node->rotation.scalar_part = 1.0;
    gltf_node->rotation.vector_part = (v3f){0.0, 0.0, 0.0};
  } else {
    gltf_node->has_trs = true;
    size_t rotation_length = json_array_length(rotation);
    if (rotation_length != 4) {
      return false;
    }

    Json *x_json = json_array_at(rotation, 0);
    if (x_json->type != JSON_NUMBER) {
      return false;
    }
    Json *y_json = json_array_at(rotation, 1);
    if (y_json->type != JSON_NUMBER) {
      return false;
    }
    Json *z_json = json_array_at(rotation, 2);
    if (z_json->type != JSON_NUMBER) {
      return false;
    }
    Json *w_json = json_array_at(rotation, 3);
    if (w_json->type != JSON_NUMBER) {
      return false;
    }

    gltf_node->rotation.vector_part =
        (v3f){.x = x_json->number, .y = y_json->number, .z = z_json->number};
    gltf_node->rotation.scalar_part = w_json->number;
  }

  JsonArray *scale = NULL;
  if (!json_object_get_array(gltf_node_json, "scale", &scale)) {
    gltf_node->scale = (v3f){1.0, 1.0, 1.0};
  } else {
    gltf_node->has_trs = true;
    size_t scale_length = json_array_length(scale);
    if (scale_length != 3) {
      return false;
    }

    Json *x_json = json_array_at(scale, 0);
    if (x_json->type != JSON_NUMBER) {
      return false;
    }
    Json *y_json = json_array_at(scale, 1);
    if (y_json->type != JSON_NUMBER) {
      return false;
    }
    Json *z_json = json_array_at(scale, 2);
    if (z_json->type != JSON_NUMBER) {
      return false;
    }

    gltf_node->scale =
        (v3f){.x = x_json->number, .y = y_json->number, .z = z_json->number};
  }

  JsonArray *translation = NULL;
  if (!json_object_get_array(gltf_node_json, "translation", &translation)) {
    gltf_node->translation = (v3f){1.0, 1.0, 1.0};
  } else {
    gltf_node->has_trs = true;
    size_t translation_length = json_array_length(translation);
    if (translation_length != 3) {
      return false;
    }

    Json *x_json = json_array_at(translation, 0);
    if (x_json->type != JSON_NUMBER) {
      return false;
    }
    Json *y_json = json_array_at(translation, 1);
    if (y_json->type != JSON_NUMBER) {
      return false;
    }
    Json *z_json = json_array_at(translation, 2);
    if (z_json->type != JSON_NUMBER) {
      return false;
    }

    gltf_node->translation =
        (v3f){.x = x_json->number, .y = y_json->number, .z = z_json->number};
  }

  JsonArray *weights = NULL;
  if (!json_object_get_array(gltf_node_json, "weights", &weights)) {
    gltf_node->weight_count = 0;
  } else {
    size_t weights_length = json_array_length(weights);
    gltf_node->weight_count = weights_length;
    gltf_node->weights = allocator_allocate_array(
        ctx->allocator, weights_length, sizeof(double));
    for (size_t weight_index = 0; weight_index < weights_length;
         weight_index++) {
      Json *weight_json = json_array_at(weights, weight_index);
      if (weight_json->type != JSON_NUMBER) {
        return false;
      }

      gltf_node->weights[weight_index] = weight_json->number;
    }
  }

  return true;
}
bool parse_meshes(const ParsingContext *ctx, Gltf *gltf,
                  const JsonArray *gltf_meshes_json) {

  ASSERT(ctx != NULL);
  ASSERT(gltf != NULL);
  ASSERT(gltf_meshes_json != NULL);

  size_t mesh_count = json_array_length(gltf_meshes_json);
  gltf->mesh_count = mesh_count;
  gltf->meshes =
      allocator_allocate_array(ctx->allocator, mesh_count, sizeof(GltfMesh));

  for (size_t mesh_index = 0; mesh_index < mesh_count; mesh_index++) {
    Json *mesh_json = json_array_at(gltf_meshes_json, mesh_index);
    if (mesh_json->type != JSON_OBJECT) {
      return false;
    }

    if (!GltfMesh_parse(ctx, &gltf->meshes[mesh_index], mesh_json->object)) {
      return false;
    }
  }

  return true;
}
bool GltfMesh_parse(const ParsingContext *ctx, GltfMesh *gltf_mesh,
                    const JsonObject *gltf_mesh_json) {
  ASSERT(ctx != NULL);
  ASSERT(gltf_mesh != NULL);
  ASSERT(gltf_mesh_json != NULL);
  LOG_DEBUG("Parsing mesh");

  char *mesh_name = NULL;
  json_object_get_string(gltf_mesh_json, "name", &mesh_name);
  if (mesh_name != NULL) {
    mesh_name = memory_clone_string(ctx->allocator, mesh_name);
    LOG_DEBUG("Mesh has name: %s", mesh_name);
  }
  gltf_mesh->name = mesh_name;

  JsonArray *weights_json = NULL;
  if (!json_object_get_array(gltf_mesh_json, "weights", &weights_json)) {
    gltf_mesh->weight_count = 0;
  } else {
    size_t weight_count = json_array_length(weights_json);
    gltf_mesh->weight_count = weight_count;
    gltf_mesh->weights =
        allocator_allocate_array(ctx->allocator, weight_count, sizeof(double));
    for (size_t weight_index = 0; weight_index < weight_count; weight_index++) {
      Json *weight_json = json_array_at(weights_json, weight_index);
      if (weight_json->type != JSON_NUMBER) {
        return false;
      }

      gltf_mesh->weights[weight_index] = weight_json->number;
    }
  }

  JsonArray *primitives_json = NULL;
  if (!json_object_get_array(gltf_mesh_json, "primitives", &primitives_json)) {
    return false;
  }

  size_t primitive_count = json_array_length(primitives_json);
  gltf_mesh->primitive_count = primitive_count;
  gltf_mesh->primitives = allocator_allocate_array(
      ctx->allocator, primitive_count, sizeof(GltfMeshPrimitive));
  for (size_t primitive_index = 0; primitive_index < primitive_count;
       primitive_index++) {
    Json *primitive_json = json_array_at(primitives_json, primitive_index);
    if (primitive_json->type != JSON_OBJECT) {
      return false;
    }

    GltfMeshPrimitive_parse(ctx, &gltf_mesh->primitives[primitive_index],
                            primitive_json->object);
  }

  return true;
}

bool GltfMeshPrimitive_parse(const ParsingContext *ctx,
                             GltfMeshPrimitive *gltf_mesh_primitive,
                             const JsonObject *gltf_mesh_primitive_json) {
  ASSERT(ctx != NULL);
  ASSERT(gltf_mesh_primitive != NULL);
  ASSERT(gltf_mesh_primitive_json != NULL);

  JsonObject *attributes_json;
  json_object_get_object(gltf_mesh_primitive_json, "attributes",
                         &attributes_json);
  size_t attribute_count = json_object_get_key_count(attributes_json);
  gltf_mesh_primitive->attribute_count = attribute_count;
  gltf_mesh_primitive->attributes = allocator_allocate_array(
      ctx->allocator, attribute_count, sizeof(GltfMeshPrimitiveAttribute));
  for (size_t i = 0; i < attribute_count; i++) {
    char *key = json_object_get_key(attributes_json, i);
    double value;
    if (!json_object_get_number(attributes_json, key, &value)) {
      return false;
    }

    gltf_mesh_primitive->attributes[i].name =
        memory_clone_string(ctx->allocator, key);
    gltf_mesh_primitive->attributes[i].accessor = (int)value;
  }

  double indices_accessor;
  if (!json_object_get_number(gltf_mesh_primitive_json, "indices",
                              &indices_accessor)) {
    gltf_mesh_primitive->has_indices = false;
  } else {
    gltf_mesh_primitive->has_indices = true;
    gltf_mesh_primitive->indices = (size_t)indices_accessor;
  }
  double material_accessor;
  if (!json_object_get_number(gltf_mesh_primitive_json, "material",
                              &material_accessor)) {
    gltf_mesh_primitive->has_material = false;
  } else {
    gltf_mesh_primitive->has_material = true;
    gltf_mesh_primitive->material = (size_t)material_accessor;
  }

  double mode_accessor;
  if (!json_object_get_number(gltf_mesh_primitive_json, "mode",
                              &mode_accessor)) {
    gltf_mesh_primitive->mode = 4;
  } else {
    gltf_mesh_primitive->mode = (size_t)mode_accessor;
  }

  return true;
}
bool parse_accessors(const ParsingContext *ctx, Gltf *gltf, size_t buffer_size,
                     const u8 *buffer, size_t buffer_view_count,
                     GltfBufferView *buffer_views,
                     const JsonArray *gltf_accessors_json) {

  ASSERT(ctx != NULL);
  ASSERT(gltf != NULL);
  ASSERT(gltf_accessors_json != NULL);

  size_t accessor_count = json_array_length(gltf_accessors_json);
  gltf->accessor_count = accessor_count;
  gltf->accessors = allocator_allocate_array(ctx->allocator, accessor_count,
                                             sizeof(GltfAccessor));

  for (size_t accessor_index = 0; accessor_index < accessor_count;
       accessor_index++) {
    Json *accessor_json = json_array_at(gltf_accessors_json, accessor_index);
    if (accessor_json->type != JSON_OBJECT) {
      return false;
    }

    if (!GltfAccessor_parse(ctx, &gltf->accessors[accessor_index], buffer_size,
                            buffer, buffer_view_count, buffer_views,
                            accessor_json->object)) {
      return false;
    }
  }

  return true;
}

bool GltfAccessor_parse(const ParsingContext *ctx, GltfAccessor *gltf_accessor,
                        size_t buffer_size, const u8 *buffer,
                        size_t buffer_view_count, GltfBufferView *buffer_views,
                        const JsonObject *gltf_accessor_json) {
  ASSERT(ctx != NULL);
  ASSERT(gltf_accessor != NULL);
  ASSERT(gltf_accessor_json != NULL);

  char *name;
  if (json_object_get_string(gltf_accessor_json, "name", &name)) {
    gltf_accessor->name = memory_clone_string(ctx->allocator, name);
  } else {
    gltf_accessor->name = NULL;
  }

  double byte_offset = 0.0;
  json_object_get_number(gltf_accessor_json, "byteOffset", &byte_offset);

  double buffer_view_d = 0.0;
  if (!json_object_get_number(gltf_accessor_json, "bufferView",
                              &buffer_view_d)) {
    return false;
  }

  size_t buffer_view_index = (size_t)buffer_view_d;
  if (buffer_view_index >= buffer_view_count) {
    return false;
  }

  GltfBufferView *buffer_view = &buffer_views[(size_t)buffer_view_index];

  size_t buffer_offset = buffer_view->byte_offset + (size_t)byte_offset;
  if (buffer_offset >= buffer_size) {
    return false;
  }

  gltf_accessor->data_ptr = &buffer[buffer_offset];
  gltf_accessor->byte_length = buffer_view->byte_length;
  if (buffer_view->has_byte_stride) {
    gltf_accessor->has_byte_stride = true;
    gltf_accessor->byte_stride = buffer_view->byte_stride;
  } else {
    gltf_accessor->has_byte_stride = false;
  }

  if (buffer_view->has_target) {
    gltf_accessor->has_buffer_view_target = true;
    gltf_accessor->buffer_view_target = buffer_view->target;
  } else {
    gltf_accessor->has_buffer_view_target = false;
  }

  double component_type;
  if (!json_object_get_number(gltf_accessor_json, "componentType",
                              &component_type)) {
    return false;
  }
  gltf_accessor->component_type = (int)component_type;

  bool normalized;
  if (!json_object_get_boolean(gltf_accessor_json, "normalized", &normalized)) {
    gltf_accessor->normalized = false;
  } else {
    gltf_accessor->normalized = normalized;
  }

  double count;
  if (!json_object_get_number(gltf_accessor_json, "count", &count)) {
    return false;
  }
  gltf_accessor->count = (int)count;

  JsonArray *max_json;
  if (!json_object_get_array(gltf_accessor_json, "max", &max_json)) {
    gltf_accessor->has_max = false;
  } else {
    gltf_accessor->has_max = true;
    size_t length = json_array_length(max_json);
    if (length > 16) {
      return false;
    }

    for (size_t i = 0; i < length; i++) {
      Json *value = json_array_at(max_json, i);
      if (value->type != JSON_NUMBER) {
        return false;
      }

      gltf_accessor->max[i] = value->number;
    }
  }

  JsonArray *min_json;
  if (!json_object_get_array(gltf_accessor_json, "min", &min_json)) {
    gltf_accessor->has_min = false;
  } else {
    gltf_accessor->has_min = true;
    size_t length = json_array_length(min_json);
    if (length > 16) {
      return false;
    }

    for (size_t i = 0; i < length; i++) {
      Json *value = json_array_at(min_json, i);
      if (value->type != JSON_NUMBER) {
        return false;
      }

      gltf_accessor->min[i] = value->number;
    }
  }

  return true;
}
bool parse_buffer_views(const ParsingContext *ctx,
                        size_t *out_buffer_view_count,
                        GltfBufferView **out_buffer_views,
                        const JsonArray *gltf_buffer_views_json) {

  ASSERT(ctx != NULL);
  ASSERT(out_buffer_view_count != NULL);
  ASSERT(out_buffer_views != NULL);
  ASSERT(gltf_buffer_views_json != NULL);

  size_t buffer_view_count = json_array_length(gltf_buffer_views_json);
  *out_buffer_view_count = buffer_view_count;
  *out_buffer_views = allocator_allocate_array(
      ctx->allocator, buffer_view_count, sizeof(GltfBufferView));

  for (size_t buffer_view_index = 0; buffer_view_index < buffer_view_count;
       buffer_view_index++) {
    Json *buffer_view_json =
        json_array_at(gltf_buffer_views_json, buffer_view_index);
    if (buffer_view_json->type != JSON_OBJECT) {
      return false;
    }

    if (!GltfBufferView_parse(ctx, &(*out_buffer_views)[buffer_view_index],
                              buffer_view_json->object)) {
      return false;
    }
  }

  return true;
}
bool GltfBufferView_parse(const ParsingContext *ctx,
                          GltfBufferView *gltf_buffer_view,
                          const JsonObject *gltf_buffer_view_json) {
  ASSERT(ctx != NULL);
  ASSERT(gltf_buffer_view != NULL);
  ASSERT(gltf_buffer_view_json != NULL);

  char *name;
  if (json_object_get_string(gltf_buffer_view_json, "name", &name)) {
    gltf_buffer_view->name = memory_clone_string(ctx->allocator, name);
  } else {
    gltf_buffer_view->name = NULL;
  }

  double buffer;
  if (!json_object_get_number(gltf_buffer_view_json, "buffer", &buffer)) {
    return false;
  }
  gltf_buffer_view->buffer = (size_t)buffer;

  double byte_offset;
  if (!json_object_get_number(gltf_buffer_view_json, "byteOffset",
                              &byte_offset)) {
    gltf_buffer_view->byte_offset = 0;
  } else {
    gltf_buffer_view->byte_offset = (size_t)byte_offset;
  }

  double byte_length;
  if (!json_object_get_number(gltf_buffer_view_json, "byteLength",
                              &byte_length)) {
    return false;
  }
  gltf_buffer_view->byte_length = (size_t)byte_length;

  double byte_stride;
  if (!json_object_get_number(gltf_buffer_view_json, "byteStride",
                              &byte_stride)) {
    gltf_buffer_view->has_byte_stride = false;
  } else {
    gltf_buffer_view->has_byte_stride = true;
    gltf_buffer_view->byte_stride = (size_t)byte_stride;
  }

  double target;
  if (!json_object_get_number(gltf_buffer_view_json, "target", &target)) {
    gltf_buffer_view->has_target = false;
  } else {
    gltf_buffer_view->has_target = true;
    gltf_buffer_view->target = (size_t)target;
  }

  return true;
}
void GltfBuffer_deinit(Allocator *allocator, GltfBuffer *gltf_buffer) {
  ASSERT(allocator != NULL);
  ASSERT(gltf_buffer != NULL);

  allocator_free(allocator, gltf_buffer->name);
}

void GltfBufferView_deinit(Allocator *allocator,
                           GltfBufferView *gltf_buffer_view) {
  ASSERT(allocator != NULL);
  ASSERT(gltf_buffer_view != NULL);

  allocator_free(allocator, gltf_buffer_view->name);
}

void GltfAccessor_deinit(Allocator *allocator, GltfAccessor *gltf_accessor) {
  ASSERT(allocator != NULL);
  ASSERT(gltf_accessor != NULL);
  if (gltf_accessor->name != NULL) {
    allocator_free(allocator, gltf_accessor->name);
  }
}
void GltfScene_deinit(Allocator *allocator, GltfScene *gltf_scene) {
  ASSERT(allocator != NULL);
  ASSERT(gltf_scene != NULL);
  allocator_free(allocator, gltf_scene->name);
  allocator_free(allocator, gltf_scene->nodes);
}
void GltfNode_deinit(Allocator *allocator, GltfNode *gltf_node) {
  ASSERT(allocator != NULL);
  ASSERT(gltf_node != NULL);
  allocator_free(allocator, gltf_node->name);
  if (gltf_node->children_count > 0) {
    allocator_free(allocator, gltf_node->children);
  }
  if (gltf_node->weight_count > 0) {
    allocator_free(allocator, gltf_node->weights);
  }
}
void GltfMesh_deinit(Allocator *allocator, GltfMesh *gltf_mesh) {
  ASSERT(allocator != NULL);
  ASSERT(gltf_mesh != NULL);
  allocator_free(allocator, gltf_mesh->name);
  if (gltf_mesh->weight_count > 0) {
    allocator_free(allocator, gltf_mesh->weights);
  }

  for (size_t primitive_index = 0; primitive_index < gltf_mesh->primitive_count;
       primitive_index++) {
    GltfMeshPrimitive_deinit(allocator,
                             &gltf_mesh->primitives[primitive_index]);
  }
  allocator_free(allocator, gltf_mesh->primitives);
}

void GltfMeshPrimitive_deinit(Allocator *allocator,
                              GltfMeshPrimitive *gltf_mesh_primitive) {
  ASSERT(allocator != NULL);
  ASSERT(gltf_mesh_primitive != NULL);
  for (size_t primitive_attribute_index = 0;
       primitive_attribute_index < gltf_mesh_primitive->attribute_count;
       primitive_attribute_index++) {
    GltfMeshPrimitiveAttribute *primitive_attribute =
        &gltf_mesh_primitive->attributes[primitive_attribute_index];
    allocator_free(allocator, primitive_attribute->name);
  }
  allocator_free(allocator, gltf_mesh_primitive->attributes);
}
GltfMeshPrimitiveAttribute *gltf_mesh_primitive_attribute_by_name(
    const char *name, GltfMeshPrimitiveAttribute *attribute_array,
    size_t attribute_count) {
  for (size_t i = 0; i < attribute_count; i++) {
    GltfMeshPrimitiveAttribute *current_attribute = &attribute_array[i];
    if (strcmp(current_attribute->name, name) == 0) {
      return current_attribute;
    }
  }

  return NULL;
}
