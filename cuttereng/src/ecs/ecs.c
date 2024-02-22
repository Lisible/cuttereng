#include "ecs.h"
#include "../assert.h"
#include "../bitset.h"
#include "../log.h"
#include "../memory.h"
#include "../transform.h"
#include "src/bytes.h"
#include "src/gltf.h"
#include "src/graphics/mesh_instance.h"
#include "src/renderer/material.h"
#include "src/vec.h"
#include <stdlib.h>
#include <string.h>

DEF_VEC(EcsSystem, EcsSystemVec, 512)
DEF_VEC(EcsCommand, EcsCommandVec, 128)

struct EcsQuery {
  char **components;
  size_t component_count;
};
EcsQuery *EcsQuery_new(Allocator *allocator,
                       const EcsQueryDescriptor *query_descriptor) {
  ASSERT(allocator != NULL);
  ASSERT(query_descriptor != NULL);
  EcsQuery *query = allocator_allocate(allocator, sizeof(EcsQuery));
  if (!query) {
    LOG_ERROR("Couldn't allocate query");
    goto err;
  }

  query->component_count = query_descriptor->component_count;
  query->components = allocator_allocate_array(
      allocator, query->component_count, sizeof(char *));
  if (!query->components) {
    LOG_ERROR("Couldn't allocate query component array");
    goto cleanup_query;
  }

  size_t component_index = 0;
  for (component_index = 0; component_index < query_descriptor->component_count;
       component_index++) {
    query->components[component_index] = memory_clone_string(
        allocator, query_descriptor->components[component_index]);
    if (!query->components[component_index]) {
      LOG_ERROR("Couldn't allocate query component");
      goto cleanup_query_components;
    }
  }

  return query;

cleanup_query_components:
  for (size_t i = 0; i < component_index; i++) {
    allocator_free(allocator, query->components[i]);
  }
  allocator_free(allocator, query->components);
cleanup_query:
  allocator_free(allocator, query);
err:
  return NULL;
}

void EcsQuery_destroy(EcsQuery *query, Allocator *allocator) {
  ASSERT(query != NULL);
  ASSERT(allocator != NULL);
  for (size_t component_index = 0; component_index < query->component_count;
       component_index++) {
    allocator_free(allocator, query->components[component_index]);
  }
  allocator_free(allocator, query->components);
  allocator_free(allocator, query);
}
size_t EcsQuery_component_count(const EcsQuery *query) {
  ASSERT(query != NULL);
  return query->component_count;
}
char *EcsQuery_component(const EcsQuery *query, size_t component_index) {
  ASSERT(query != NULL);
  ASSERT(component_index < query->component_count);
  return query->components[component_index];
}

// FIXME Component stores are not growable so far, this needs to be fixed
// The data block should be reallocated with a larger capacity, the bitset too.
struct ComponentStore {
  Allocator *allocator;
  void *data;
  u8 *bitset;
  size_t length;
  size_t capacity;
  size_t item_size;
};

ComponentStore *component_store_new(Allocator *allocator, size_t item_size) {
  ASSERT(allocator != NULL);
  const size_t INITIAL_CAPACITY = 32;
  ComponentStore *store = allocator_allocate(allocator, sizeof(ComponentStore));
  if (!store)
    goto err;

  store->allocator = allocator;
  store->capacity = INITIAL_CAPACITY;
  store->length = 0;
  store->item_size = item_size;
  store->data = NULL;
  if (item_size > 0) {
    store->data =
        allocator_allocate_array(allocator, store->capacity, store->item_size);
    if (!store->data) {
      LOG_ERROR("ComponentStore allocation failed");
      goto err;
    }
  }
  store->bitset = allocator_allocate_array(
      allocator, BITNSLOTS(INITIAL_CAPACITY), sizeof(u8));
  if (!store->bitset) {
    LOG_ERROR("ComponentStore bitset allocation failed");
    goto cleanup_data;
  }

  return store;
cleanup_data:
  allocator_free(allocator, store->data);
err:
  return NULL;
}

void component_store_destroy(Allocator *allocator, ComponentStore *store) {
  ASSERT(store != NULL);
  (void)allocator;
  if (store->data != NULL) {
    allocator_free(store->allocator, store->data);
  }
  allocator_free(store->allocator, store->bitset);
  allocator_free(store->allocator, store);
}
DEF_HASH_TABLE(ComponentStore *, HashTableComponentStore,
               component_store_destroy)

void component_store_ensure_capacity(ComponentStore *store, size_t capacity) {
  ASSERT(store != NULL);
  LOG_TRACE("Ensuring component store capacity");
  if (store->capacity > capacity) {
    LOG_TRACE("Requested capacity is smaller than store capacity");
    return;
  }

  size_t new_capacity = store->capacity * 2;
  while (new_capacity <= capacity) {
    new_capacity *= 2;
  }

  if (store->item_size != 0) {
    LOG_TRACE(
        "Reallocating component store data buffer from capacity: %d, size: "
        "%d to capacity: %d, size: %d",
        store->capacity, store->capacity * store->item_size, new_capacity,
        new_capacity * store->item_size);

    store->data = allocator_reallocate(store->allocator, store->data,
                                       store->capacity * store->item_size,
                                       new_capacity * store->item_size);
    if (!store->data) {
      PANIC("Couldn't reallocate component store buffer from capacity %d "
            "to %d",
            store->capacity, new_capacity);
    }
  }

  LOG_TRACE(
      "Reallocating component store bitset buffer from size: %d, to size: %d",
      BITNSLOTS(store->capacity), BITNSLOTS(new_capacity));
  store->bitset = allocator_reallocate(store->allocator, store->bitset,
                                       BITNSLOTS(store->capacity) * sizeof(u8),
                                       BITNSLOTS(new_capacity) * sizeof(u8));
  if (!store->bitset) {
    PANIC("Couldn't reallocate component store bitset from capacity %d "
          "to %d",
          store->capacity, new_capacity);
  }
  store->capacity = new_capacity;
}

void component_store_set(ComponentStore *store, EcsId entity_id,
                         const void *data) {
  ASSERT(store != NULL);
  ASSERT(data != NULL || store->item_size == 0);
  LOG_TRACE("Setting component of size %d for entity: %d", store->item_size,
            entity_id);

  if (entity_id >= store->capacity) {
    component_store_ensure_capacity(store, entity_id);
  }

  if (store->item_size > 0) {
    LOG_DEBUG("entity_id: %d", entity_id);
    LOG_DEBUG("store->item_size: %d", store->item_size);
    char *store_data_ptr = &((char *)store->data)[entity_id * store->item_size];
    memmove(store_data_ptr, (char *)data, store->item_size * sizeof(char));
  }
  BITSET(store->bitset, entity_id);
}

void *component_store_get(ComponentStore *store, EcsId entity_id) {
  ASSERT(store != NULL);

  if (!BITTEST(store->bitset, entity_id)) {
    return NULL;
  }

  char *ptr = (char *)store->data;
  return ptr + (entity_id * store->item_size);
}
void ecs_register_system_(Ecs *ecs, EcsSystem *system) {
  EcsSystemVec_append(&ecs->systems, system, 1);
}
void ecs_command_execute(Ecs *ecs, EcsCommand *command) {
  switch (command->type) {
  case EcsCommandType_RegisterSystem:
    ecs_register_system_(ecs, &command->register_system.system);
    break;
  case EcsCommandType_CreateEntity:
    ecs_create_entity(ecs);
    break;
  case EcsCommandType_InsertComponent: {
    EcsInsertComponentCommand *insert_component_command =
        &command->insert_component;
    ecs_insert_component_(ecs, insert_component_command->entity,
                          insert_component_command->component_name,
                          insert_component_command->component_size,
                          insert_component_command->component_data);
    break;
  }
  default:
    break;
  }
}
void ecs_command_deinit(Allocator *allocator, EcsCommand *command) {
  switch (command->type) {
  case EcsCommandType_InsertComponent:
    allocator_free(allocator, command->insert_component.component_name);
    allocator_free(allocator, command->insert_component.component_data);
    break;
  default:
    break;
  }
}

void ecs_command_queue_init(Allocator *allocator, EcsCommandQueue *queue,
                            Ecs *ecs) {
  ASSERT(allocator != NULL);
  ASSERT(queue != NULL);

  queue->allocator = allocator;
  queue->ecs = ecs;
  EcsCommandVec_init(allocator, &queue->commands);
}

void ecs_command_queue_deinit(EcsCommandQueue *queue) {
  ASSERT(queue != NULL);
  EcsCommandVec_deinit(&queue->commands);
}

void ecs_command_queue_register_system(EcsCommandQueue *queue,
                                       const EcsSystemDescriptor *system) {
  ASSERT(queue != NULL);
  ASSERT(system != NULL);

  EcsCommand command;
  command.type = EcsCommandType_RegisterSystem;
  command.register_system.system.fn = system->fn;
  EcsQuery *query = EcsQuery_new(queue->allocator, &system->query);
  command.register_system.system.query = query;
  EcsCommandVec_append(&queue->commands, &command, 1);
}
EcsId ecs_command_queue_create_entity(EcsCommandQueue *queue) {
  ASSERT(queue != NULL);
  EcsCommand command = {.type = EcsCommandType_CreateEntity};
  EcsId entity_id = ecs_reserve_entity(queue->ecs);
  EcsCommandVec_append(&queue->commands, &command, 1);
  return entity_id;
}
EcsId ecs_command_queue_import_glb(EcsCommandQueue *queue, Assets *assets,
                                   const char *glb_path) {
  ASSERT(queue != NULL);
  ASSERT(glb_path != NULL);

  char *effective_path = asset_get_effective_path(queue->allocator, glb_path);
  if (!effective_path) {
    goto err;
  }
  LOG_DEBUG("Importing GLB from %s", glb_path);
  FILE *glb_file = fopen(effective_path, "r");
  if (!glb_file) {
    LOG_ERROR("Couldn't open GLB file");
    goto cleanup_effective_path;
  }
  allocator_free(queue->allocator, effective_path);

  if (fseek(glb_file, 0, SEEK_END) < 0) {
    LOG_ERROR("Couldn't seek the end of the GLB file");
    goto cleanup_glb_file;
  }
  long glb_file_length = ftell(glb_file);
  if (fseek(glb_file, 0, SEEK_SET) < 0) {
    LOG_ERROR("Couldn't seek the beginning of the GLB file");
    goto cleanup_glb_file;
  }

  u8 *glb_file_content = allocator_allocate(queue->allocator, glb_file_length);
  if (!glb_file_content) {
    LOG_ERROR("Couldn't allocate GLB file content buffer");
    goto cleanup_glb_file;
  }

  fread(glb_file_content, 1, glb_file_length, glb_file);

  if (fclose(glb_file) == EOF) {
    LOG_ERROR("An error occured while closing the GLB file");
    goto cleanup_glb_file;
  }

  Gltf *gltf =
      Gltf_parse_glb(queue->allocator, glb_file_content, glb_file_length);
  if (!gltf) {
    LOG_ERROR("Couldn't parse GLB");
    goto err;
  }

  if (gltf->scene_count != 1) {
    LOG_ERROR("Only single scene GLB are supported so far");
    goto cleanup_gltf;
  }

  GltfScene *scene = &gltf->scenes[0];
  EcsId *node_entities = allocator_allocate_array(
      queue->allocator, gltf->node_count, sizeof(EcsId));

  LOG_DEBUG("Scene name: %s", scene->name);
  for (size_t node_index = 0; node_index < gltf->node_count; node_index++) {
    GltfNode *node = &gltf->nodes[node_index];
    EcsId entity_id = ecs_command_queue_create_entity(queue);
    node_entities[node_index] = entity_id;
    Transform transform = TRANSFORM_DEFAULT;
    if (node->has_trs) {
      transform = (Transform){.position = node->translation,
                              .rotation = node->rotation,
                              .scale = node->scale};
    } else {
      // TODO transform from matrix
    }

    ecs_command_queue_insert_component_with_ptr(queue, entity_id, Transform,
                                                &transform);
    if (node->has_mesh) {
      GltfMesh *gltf_mesh = &gltf->meshes[node->mesh];
      GltfMeshPrimitive *primitive = &gltf_mesh->primitives[0];
      GltfMeshPrimitiveAttribute *position_attribute =
          gltf_mesh_primitive_attribute_by_name(
              "POSITION", primitive->attributes, primitive->attribute_count);
      GltfAccessor *position_accessor =
          &gltf->accessors[position_attribute->accessor];
      GltfMeshPrimitiveAttribute *normal_attribute =
          gltf_mesh_primitive_attribute_by_name("NORMAL", primitive->attributes,
                                                primitive->attribute_count);
      GltfAccessor *normal_accessor =
          normal_attribute == NULL
              ? NULL
              : &gltf->accessors[normal_attribute->accessor];
      GltfMeshPrimitiveAttribute *texture_coordinates_attribute =
          gltf_mesh_primitive_attribute_by_name(
              "TEXCOORD_0", primitive->attributes, primitive->attribute_count);
      GltfAccessor *texture_coordinates_accessor =
          texture_coordinates_attribute == NULL
              ? NULL
              : &gltf->accessors[texture_coordinates_attribute->accessor];

      Mesh *mesh = allocator_allocate(queue->allocator, sizeof(Mesh));

      mesh->vertex_count = position_accessor->count;
      mesh->vertices = allocator_allocate_array(
          queue->allocator, position_accessor->count, sizeof(Vertex));
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
      if (primitive->has_indices) {
        GltfAccessor *index_accessor = &gltf->accessors[primitive->indices];
        size_t index_count = index_accessor->count;
        mesh->index_count = index_count;
        mesh->indices = allocator_allocate_array(queue->allocator, index_count,
                                                 sizeof(Index));
        for (size_t index_index = 0; index_index < index_count; index_index++) {
          size_t stride = index_accessor->byte_stride;
          if (stride == 0) {
            stride = sizeof(Index);
          }
          mesh->indices[index_index] = u16_from_bytes_le(
              &index_accessor->data_ptr[index_index * stride]);
        }
      }

      AssetHandle mesh_handle = assets_store(assets, Mesh, node->name, mesh);

      ecs_command_queue_insert_component(queue, entity_id, MeshInstance,
                                         {.mesh_handle = mesh_handle});
      ecs_command_queue_insert_component(queue, entity_id, Material,
                                         {.base_color = 0, .normal = 0});
    }
    LOG_DEBUG("Node %d name: %s", node_index, node->name);
  }

  return node_entities[scene->nodes[0]];

cleanup_gltf:
  Gltf_destroy(queue->allocator, gltf);
cleanup_glb_file:
  fclose(glb_file);
cleanup_effective_path:
  allocator_free(queue->allocator, effective_path);
err:
  PANIC();
}
void ecs_command_queue_insert_component_(EcsCommandQueue *queue, EcsId entity,
                                         char *component_name,
                                         size_t component_size,
                                         void *component_data) {
  ASSERT(queue != NULL);
  ASSERT(component_name != NULL);
  ASSERT(component_data != NULL);

  char *owned_component_name =
      memory_clone_string(queue->allocator, component_name);
  void *owned_component_data =
      allocator_allocate(queue->allocator, component_size);
  memcpy(owned_component_data, component_data, component_size);

  EcsCommand command = {.type = EcsCommandType_InsertComponent,
                        .insert_component = (EcsInsertComponentCommand){
                            .entity = entity,
                            .component_name = owned_component_name,
                            component_size,
                            .component_data = owned_component_data}};
  EcsCommandVec_append(&queue->commands, &command, 1);
}

void ecs_command_queue_insert_tag_component_(EcsCommandQueue *queue,
                                             EcsId entity,
                                             char *component_name) {
  ASSERT(queue != NULL);
  ASSERT(component_name != NULL);
  char *owned_component_name =
      memory_clone_string(queue->allocator, component_name);

  EcsCommand command = {.type = EcsCommandType_InsertComponent,
                        .insert_component = (EcsInsertComponentCommand){
                            .entity = entity,
                            .component_name = owned_component_name,
                            0,
                            .component_data = NULL}};
  EcsCommandVec_append(&queue->commands, &command, 1);
}
void ecs_command_queue_finish(Ecs *ecs, EcsCommandQueue *queue) {
  ASSERT(queue != NULL);
  for (size_t command_index = 0; command_index < queue->commands.length;
       command_index++) {
    ecs_command_execute(ecs, &queue->commands.data[command_index]);
    ecs_command_deinit(queue->allocator, &queue->commands.data[command_index]);
  }
  EcsCommandVec_clear(&queue->commands);
  ecs->reserved_entity_count = 0;
}

void ecs_default_init_system(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  (void)it;
}
void ecs_init(Allocator *allocator, Ecs *ecs, EcsSystemFn init_system,
              void *system_context) {
  ASSERT(allocator != NULL);
  ASSERT(ecs != NULL);

  ecs->allocator = allocator;
  ecs->entity_count = 0;
  ecs->reserved_entity_count = 0;
  ecs->component_stores = HashTableComponentStore_create(allocator, 16);
  EcsSystemVec_init(allocator, &ecs->systems);
  ecs_command_queue_init(allocator, &ecs->command_queue, ecs);
  EcsQueryIt it = {.allocator = ecs->allocator, .ctx = system_context};
  init_system(&ecs->command_queue, &it);
}
void ecs_deinit(Ecs *ecs) {
  ecs_command_queue_deinit(&ecs->command_queue);

  for (size_t i = 0; i < ecs->systems.length; i++) {
    EcsQuery_destroy(ecs->systems.data[i].query, ecs->allocator);
  }
  EcsSystemVec_deinit(&ecs->systems);
  HashTableComponentStore_destroy(ecs->component_stores);
}
void ecs_register_system(Ecs *ecs,
                         const EcsSystemDescriptor *system_descriptor) {

  EcsQuery *query = EcsQuery_new(ecs->allocator, &system_descriptor->query);
  EcsSystem system = {.fn = system_descriptor->fn, .query = query};
  ecs_register_system_(ecs, &system);
}

void ecs_run_systems(Ecs *ecs, const void *system_context) {
  ASSERT(ecs != NULL);
  for (size_t i = 0; i < ecs->systems.length; i++) {
    EcsQueryIt it = ecs_query(ecs, ecs->systems.data[i].query);
    it.ctx = system_context;
    ecs->systems.data[i].fn(&ecs->command_queue, &it);
    ecs_query_it_deinit(&it);
  }
}
void ecs_process_command_queue(Ecs *ecs) {
  ASSERT(ecs != NULL);
  ecs_command_queue_finish(ecs, &ecs->command_queue);
}

EcsId ecs_create_entity(Ecs *ecs) {
  ASSERT(ecs != NULL);

  EcsId id = ecs->entity_count;
  ecs->entity_count++;
  return id;
}
EcsId ecs_reserve_entity(Ecs *ecs) {
  ASSERT(ecs != NULL);
  EcsId reserved_entity_id = ecs->entity_count + ecs->reserved_entity_count;
  ecs->reserved_entity_count++;
  return reserved_entity_id;
}

size_t ecs_get_entity_count(const Ecs *ecs) {
  ASSERT(ecs != NULL);
  return ecs->entity_count;
}

void ecs_insert_component_(Ecs *ecs, EcsId entity_id, char *component_name,
                           size_t component_size, const void *data) {
  ASSERT(ecs != NULL);
  ASSERT(data != NULL || component_size == 0);

  LOG_DEBUG("Inserting component %s for entity %d", component_name, entity_id);
  if (!HashTableComponentStore_has(ecs->component_stores, component_name)) {
    LOG_DEBUG("Component store not found for component %s, creating it",
              component_name);
    ComponentStore *store = component_store_new(ecs->allocator, component_size);
    if (!store) {
      goto err;
    }
    LOG_DEBUG("Component store successfully created");
    HashTableComponentStore_set(ecs->component_stores, component_name, store);
  }

  ComponentStore *store =
      HashTableComponentStore_get(ecs->component_stores, component_name);
  component_store_set(store, entity_id, data);
err:
  return;
}

bool ecs_has_component_(const Ecs *ecs, EcsId entity_id,
                        const char *component_name) {
  ASSERT(ecs != NULL);

  ComponentStore *store =
      HashTableComponentStore_get(ecs->component_stores, component_name);
  if (!store)
    return false;

  return component_store_get(store, entity_id) != NULL;
}

void *ecs_get_component_(const Ecs *ecs, EcsId entity_id,
                         const char *component_name) {
  ASSERT(ecs != NULL);

  ComponentStore *store =
      HashTableComponentStore_get(ecs->component_stores, component_name);

  if (!store)
    return NULL;

  return component_store_get(store, entity_id);
}

size_t ecs_count_matching(const Ecs *ecs, const EcsQuery *query) {
  ASSERT(ecs != NULL);
  ASSERT(query != NULL);

  size_t result = 0;

  // FIXME this is O(n + m) with n being the entity count and m being the
  // query component count. Using a hierarchical bitset could help.
  for (size_t entity_id = 0; entity_id < ecs_get_entity_count(ecs);
       entity_id++) {
    if (ecs_query_is_matching(ecs, query, entity_id))
      result++;
  }

  return result;
}

bool ecs_query_is_matching(const Ecs *ecs, const EcsQuery *query,
                           EcsId entity_id) {
  ASSERT(ecs != NULL);
  ASSERT(query != NULL);

  bool matches = true;
  size_t component_index = 0;
  while (component_index < query->component_count &&
         query->components[component_index] != NULL) {
    ComponentStore *component_store = HashTableComponentStore_get(
        ecs->component_stores, query->components[component_index]);

    if (!component_store || component_store->capacity <= entity_id) {
      return false;
    }
    matches = matches && BITTEST(component_store->bitset, entity_id);
    component_index++;
  }

  return matches;
}

struct EcsQueryItState {
  ComponentStore *component_stores[ECS_QUERY_MAX_COMPONENT_COUNT];
  EcsId *matching_entities;
  size_t current_matching_entity;
  size_t matching_entity_count;
  bool iterating;
};

EcsQueryIt ecs_query(const Ecs *ecs, const EcsQuery *query) {
  ASSERT(ecs != NULL);
  ASSERT(query != NULL);

  EcsQueryIt iterator = {0};
  iterator.allocator = ecs->allocator;
  iterator.state = allocator_allocate(ecs->allocator, sizeof(EcsQueryItState));
  ASSERT(iterator.state != NULL);
  memset(iterator.state, 0, sizeof(EcsQueryItState));
  iterator.state->iterating = false;
  iterator.state->matching_entities = allocator_allocate_array(
      ecs->allocator, ecs->entity_count, sizeof(size_t));
  ASSERT(iterator.state->matching_entities != NULL);

  for (size_t i = 0; i < query->component_count; i++) {
    ASSERT(i < ECS_QUERY_MAX_COMPONENT_COUNT - 1,
           "Too many components in the query");
    ComponentStore *component_store = HashTableComponentStore_get(
        ecs->component_stores, query->components[i]);
    iterator.state->component_stores[i] = component_store;
  }

  for (EcsId i = 0; i < ecs->entity_count; i++) {
    if (ecs_query_is_matching(ecs, query, i)) {
      iterator.state->matching_entities[iterator.state->matching_entity_count] =
          i;
      iterator.state->matching_entity_count++;
    }
  }

  return iterator;
}

bool ecs_query_it_next(EcsQueryIt *it) {
  ASSERT(it != NULL);
  ASSERT(it->state != NULL);

  if (it->state->iterating == false) {
    it->state->iterating = true;
  } else {
    it->state->current_matching_entity++;
  }

  if (it->state->current_matching_entity >= it->state->matching_entity_count) {
    ecs_query_it_deinit(it);
    return false;
  }

  return true;
}
void ecs_query_it_deinit(EcsQueryIt *it) {
  ASSERT(it != NULL);
  if (it->state == NULL)
    return;
  allocator_free(it->allocator, it->state->matching_entities);
  allocator_free(it->allocator, it->state);
  it->state = NULL;
  it->allocator = NULL;
}
void *ecs_query_it_get_(const EcsQueryIt *it, size_t component) {
  ASSERT(it != NULL);
  ASSERT(it->state != NULL);

  if (!it->state->component_stores[component])
    return NULL;

  return component_store_get(
      it->state->component_stores[component],
      it->state->matching_entities[it->state->current_matching_entity]);
}
EcsId ecs_query_it_entity_id(const EcsQueryIt *it) {
  ASSERT(it != NULL);
  return it->state->matching_entities[it->state->current_matching_entity];
}
