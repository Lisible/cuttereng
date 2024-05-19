#include "ecs.h"
#include "../common.h"
#include <lisiblestd/assert.h>
#include <lisiblestd/bitset.h>
#include <lisiblestd/log.h>
#include <lisiblestd/memory.h>
#include <lisiblestd/vec.h>
#include <stdio.h>
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
  LSTD_ASSERT(allocator != NULL);
  LSTD_ASSERT(query_descriptor != NULL);
  EcsQuery *query = Allocator_allocate(allocator, sizeof(EcsQuery));
  if (!query) {
    LOG0_ERROR("Couldn't allocate query");
    goto err;
  }

  query->component_count = query_descriptor->component_count;
  query->components = Allocator_allocate_array(
      allocator, query->component_count, sizeof(char *));
  if (!query->components) {
    LOG0_ERROR("Couldn't allocate query component array");
    goto cleanup_query;
  }

  size_t component_index = 0;
  for (component_index = 0; component_index < query_descriptor->component_count;
       component_index++) {
    query->components[component_index] = memory_clone_string(
        allocator, query_descriptor->components[component_index]);
    if (!query->components[component_index]) {
      LOG0_ERROR("Couldn't allocate query component");
      goto cleanup_query_components;
    }
  }

  return query;

cleanup_query_components:
  for (size_t i = 0; i < component_index; i++) {
    Allocator_free(allocator, query->components[i]);
  }
  Allocator_free(allocator, query->components);
cleanup_query:
  Allocator_free(allocator, query);
err:
  return NULL;
}

void EcsQuery_destroy(EcsQuery *query, Allocator *allocator) {
  LSTD_ASSERT(query != NULL);
  LSTD_ASSERT(allocator != NULL);
  for (size_t component_index = 0; component_index < query->component_count;
       component_index++) {
    Allocator_free(allocator, query->components[component_index]);
  }
  Allocator_free(allocator, query->components);
  Allocator_free(allocator, query);
}
size_t EcsQuery_component_count(const EcsQuery *query) {
  LSTD_ASSERT(query != NULL);
  return query->component_count;
}
char *EcsQuery_component(const EcsQuery *query, size_t component_index) {
  LSTD_ASSERT(query != NULL);
  LSTD_ASSERT(component_index < query->component_count);
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
  LSTD_ASSERT(allocator != NULL);
  const size_t INITIAL_CAPACITY = 32;
  ComponentStore *store = Allocator_allocate(allocator, sizeof(ComponentStore));
  if (!store)
    goto err;

  store->allocator = allocator;
  store->capacity = INITIAL_CAPACITY;
  store->length = 0;
  store->item_size = item_size;
  store->data = NULL;
  if (item_size > 0) {
    store->data =
        Allocator_allocate_array(allocator, store->capacity, store->item_size);
    if (!store->data) {
      LOG0_ERROR("ComponentStore allocation failed");
      goto err;
    }
  }
  store->bitset = Allocator_allocate_array(
      allocator, BITNSLOTS(INITIAL_CAPACITY), sizeof(u8));
  if (!store->bitset) {
    LOG0_ERROR("ComponentStore bitset allocation failed");
    goto cleanup_data;
  }

  return store;
cleanup_data:
  Allocator_free(allocator, store->data);
err:
  return NULL;
}

void component_store_destroy(Allocator *allocator, ComponentStore *store) {
  LSTD_ASSERT(store != NULL);
  (void)allocator;
  if (store->data != NULL) {
    Allocator_free(store->allocator, store->data);
  }
  Allocator_free(store->allocator, store->bitset);
  Allocator_free(store->allocator, store);
}

void component_store_dctor(Allocator *allocator, void *store) {
  LSTD_ASSERT(allocator != NULL);
  LSTD_ASSERT(store != NULL);
  component_store_destroy(allocator, store);
}

void component_store_ensure_capacity(ComponentStore *store, size_t capacity) {
  LSTD_ASSERT(store != NULL);
  LOG0_TRACE("Ensuring component store capacity");
  if (store->capacity > capacity) {
    LOG0_TRACE("Requested capacity is smaller than store capacity");
    return;
  }

  size_t new_capacity = store->capacity * 2;
  while (new_capacity <= capacity) {
    new_capacity *= 2;
  }

  if (store->item_size != 0) {
    LOG_TRACE(
        "Reallocating component store data buffer from capacity: %zu, size: "
        "%zu to capacity: %zu, size: %zu",
        store->capacity, store->capacity * store->item_size, new_capacity,
        new_capacity * store->item_size);

    store->data = Allocator_reallocate(store->allocator, store->data,
                                       store->capacity * store->item_size,
                                       new_capacity * store->item_size);
    if (!store->data) {
      PANIC("Couldn't reallocate component store buffer from capacity %zu "
            "to %zu",
            store->capacity, new_capacity);
    }
  }

  LOG_TRACE(
      "Reallocating component store bitset buffer from size: %zu, to size: %zu",
      BITNSLOTS(store->capacity), BITNSLOTS(new_capacity));
  store->bitset = Allocator_reallocate(store->allocator, store->bitset,
                                       BITNSLOTS(store->capacity) * sizeof(u8),
                                       BITNSLOTS(new_capacity) * sizeof(u8));
  if (!store->bitset) {
    PANIC("Couldn't reallocate component store bitset from capacity %zu "
          "to %zu",
          store->capacity, new_capacity);
  }
  store->capacity = new_capacity;
}

void component_store_set(ComponentStore *store, EcsId entity_id,
                         const void *data) {
  LSTD_ASSERT(store != NULL);
  LSTD_ASSERT(data != NULL || store->item_size == 0);
  LOG_TRACE("Setting component of size %zu for entity: %zu", store->item_size,
            entity_id);

  if (entity_id >= store->capacity) {
    component_store_ensure_capacity(store, entity_id);
  }

  if (store->item_size > 0) {
    LOG_DEBUG("entity_id: %zu", entity_id);
    LOG_DEBUG("store->item_size: %zu", store->item_size);
    char *store_data_ptr = &((char *)store->data)[entity_id * store->item_size];
    memmove(store_data_ptr, (char *)data, store->item_size * sizeof(char));
  }
  BITSET(store->bitset, entity_id);
}

void *component_store_get(ComponentStore *store, EcsId entity_id) {
  LSTD_ASSERT(store != NULL);
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
  case EcsCommandType_InsertRelationship: {
    EcsInsertRelationshipCommand *insert_relationship_command =
        &command->insert_relationship;
    ecs_insert_relationship_(ecs, insert_relationship_command->source,
                             insert_relationship_command->relationship_name,
                             insert_relationship_command->target);
  }
  default:
    break;
  }
}
void ecs_command_deinit(Allocator *allocator, EcsCommand *command) {
  switch (command->type) {
  case EcsCommandType_InsertComponent:
    Allocator_free(allocator, command->insert_component.component_name);
    Allocator_free(allocator, command->insert_component.component_data);
    break;
  case EcsCommandType_InsertRelationship:
    Allocator_free(allocator, command->insert_relationship.relationship_name);
    break;
  default:
    break;
  }
}

void ecs_command_queue_init(Allocator *allocator, EcsCommandQueue *queue,
                            Ecs *ecs) {
  LSTD_ASSERT(allocator != NULL);
  LSTD_ASSERT(queue != NULL);

  queue->allocator = allocator;
  queue->ecs = ecs;
  EcsCommandVec_init(allocator, &queue->commands);
}

void ecs_command_queue_deinit(EcsCommandQueue *queue) {
  LSTD_ASSERT(queue != NULL);
  EcsCommandVec_deinit(&queue->commands);
}

void ecs_command_queue_register_system(EcsCommandQueue *queue,
                                       const EcsSystemDescriptor *system) {
  LSTD_ASSERT(queue != NULL);
  LSTD_ASSERT(system != NULL);

  EcsCommand command;
  command.type = EcsCommandType_RegisterSystem;
  command.register_system.system.fn = system->fn;
  EcsQuery *query = EcsQuery_new(queue->allocator, &system->query);
  command.register_system.system.query = query;
  EcsCommandVec_append(&queue->commands, &command, 1);
}
EcsId ecs_command_queue_create_entity(EcsCommandQueue *queue) {
  LSTD_ASSERT(queue != NULL);
  EcsCommand command = {.type = EcsCommandType_CreateEntity};
  EcsId entity_id = ecs_reserve_entity(queue->ecs);
  EcsCommandVec_append(&queue->commands, &command, 1);
  return entity_id;
}

void ecs_command_queue_insert_component_(EcsCommandQueue *queue, EcsId entity,
                                         char *component_name,
                                         size_t component_size,
                                         void *component_data) {
  LSTD_ASSERT(queue != NULL);
  LSTD_ASSERT(component_name != NULL);
  LSTD_ASSERT(component_data != NULL);

  char *owned_component_name =
      memory_clone_string(queue->allocator, component_name);
  void *owned_component_data =
      Allocator_allocate(queue->allocator, component_size);
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
  LSTD_ASSERT(queue != NULL);
  LSTD_ASSERT(component_name != NULL);
  char *owned_component_name =
      memory_clone_string(queue->allocator, component_name);

  EcsCommand command = {.type = EcsCommandType_InsertComponent,
                        .insert_component = (EcsInsertComponentCommand){
                            .entity = entity,
                            .component_name = owned_component_name,
                            .component_size = 0,
                            .component_data = NULL}};
  EcsCommandVec_append(&queue->commands, &command, 1);
}
void ecs_command_queue_insert_relationship_(EcsCommandQueue *queue,
                                            EcsId source_entity,
                                            char *relationship_name,
                                            EcsId target_entity) {
  LSTD_ASSERT(queue != NULL);
  LSTD_ASSERT(relationship_name != NULL);

  char *owned_relationship_name =
      memory_clone_string(queue->allocator, relationship_name);
  EcsCommand command = {.type = EcsCommandType_InsertRelationship,
                        .insert_relationship = (EcsInsertRelationshipCommand){
                            .source = source_entity,
                            .target = target_entity,
                            .relationship_name = owned_relationship_name}};
  EcsCommandVec_append(&queue->commands, &command, 1);
}
void ecs_command_queue_finish(Ecs *ecs, EcsCommandQueue *queue) {
  LSTD_ASSERT(queue != NULL);
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
void RelationshipStore_dctor(Allocator *allocator, void *store);
void ecs_init(Allocator *allocator, Ecs *ecs, EcsSystemFn init_system,
              void *system_context) {
  LSTD_ASSERT(allocator != NULL);
  LSTD_ASSERT(ecs != NULL);

  ecs->allocator = allocator;
  ecs->entity_count = 0;
  ecs->reserved_entity_count = 0;
  if (!HashTable_init_with_dctors(allocator, &ecs->component_stores, 16,
                                  hash_str_hash, hash_str_eq, hash_str_dctor,
                                  component_store_dctor)) {
    PANIC("Couldn't create component store hash table");
  }
  if (!HashTable_init_with_dctors(ecs->allocator, &ecs->relationship_stores, 16,
                                  hash_str_hash, hash_str_eq, hash_str_dctor,
                                  RelationshipStore_dctor)) {
    PANIC("Couldn't create relationship store hash table");
  }

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
  HashTable_deinit(&ecs->relationship_stores);
  HashTable_deinit(&ecs->component_stores);
}
void ecs_register_system(Ecs *ecs,
                         const EcsSystemDescriptor *system_descriptor) {

  EcsQuery *query = EcsQuery_new(ecs->allocator, &system_descriptor->query);
  EcsSystem system = {.fn = system_descriptor->fn, .query = query};
  ecs_register_system_(ecs, &system);
}

void ecs_run_systems(Ecs *ecs, const void *system_context) {
  LSTD_ASSERT(ecs != NULL);
  for (size_t i = 0; i < ecs->systems.length; i++) {
    EcsQueryIt it = ecs_query(ecs, ecs->systems.data[i].query);
    it.ctx = system_context;
    ecs->systems.data[i].fn(&ecs->command_queue, &it);
    ecs_query_it_deinit(&it);
  }
}
void ecs_process_command_queue(Ecs *ecs) {
  LSTD_ASSERT(ecs != NULL);
  ecs_command_queue_finish(ecs, &ecs->command_queue);
}

EcsId ecs_create_entity(Ecs *ecs) {
  LSTD_ASSERT(ecs != NULL);

  EcsId id = ecs->entity_count;
  ecs->entity_count++;
  return id;
}
EcsId ecs_reserve_entity(Ecs *ecs) {
  LSTD_ASSERT(ecs != NULL);
  EcsId reserved_entity_id = ecs->entity_count + ecs->reserved_entity_count;
  ecs->reserved_entity_count++;
  return reserved_entity_id;
}

size_t ecs_get_entity_count(const Ecs *ecs) {
  LSTD_ASSERT(ecs != NULL);
  return ecs->entity_count;
}

void ecs_insert_component_(Ecs *ecs, EcsId entity_id, char *component_name,
                           size_t component_size, const void *data) {
  LSTD_ASSERT(ecs != NULL);
  LSTD_ASSERT(data != NULL || component_size == 0);

  LOG_DEBUG("Inserting component %s for entity %zu", component_name, entity_id);
  if (!HashTable_has(&ecs->component_stores, component_name)) {
    LOG_DEBUG("Component store not found for component %s, creating it",
              component_name);
    ComponentStore *store = component_store_new(ecs->allocator, component_size);
    if (!store) {
      goto err;
    }
    LOG0_DEBUG("Component store successfully created");
    HashTable_insert(&ecs->component_stores, strdup(component_name), store);
  }

  ComponentStore *store = HashTable_get(&ecs->component_stores, component_name);
  component_store_set(store, entity_id, data);
err:
  return;
}

uint64_t ecs_id_hash_fn(const void *ecs_id) { return *(EcsId *)ecs_id; }
bool ecs_id_eq_fn(const void *a, const void *b) {
  return (*(EcsId *)a) == (*(EcsId *)b);
}
typedef struct {
  HashTable sources_for_entity;
  HashTable targets_for_entity;
} RelationshipStore;

void ecs_id_dctor_fn(Allocator *allocator, void *ecs_id) {
  LSTD_ASSERT(allocator != NULL);
  LSTD_ASSERT(ecs_id != NULL);
  Allocator_free(allocator, ecs_id);
}

void HashSet_dctor_fn(Allocator *allocator, void *set) {
  LSTD_ASSERT(allocator != NULL);
  LSTD_ASSERT(set != NULL);
  HashSet_deinit(set);
  Allocator_free(allocator, set);
}

bool RelationshipStore_init(Allocator *allocator,
                            RelationshipStore *relationship_store) {
  LSTD_ASSERT(allocator != NULL);
  LSTD_ASSERT(relationship_store != NULL);
  if (!HashTable_init_with_dctors(
          allocator, &relationship_store->sources_for_entity, 16,
          ecs_id_hash_fn, ecs_id_eq_fn, ecs_id_dctor_fn, HashSet_dctor_fn)) {
    return false;
  }
  return HashTable_init_with_dctors(
      allocator, &relationship_store->targets_for_entity, 16, ecs_id_hash_fn,
      ecs_id_eq_fn, ecs_id_dctor_fn, HashSet_dctor_fn);
}

RelationshipStore *RelationshipStore_create(Allocator *allocator) {
  LSTD_ASSERT(allocator != NULL);
  RelationshipStore *relationship_store =
      Allocator_allocate(allocator, sizeof(RelationshipStore));
  RelationshipStore_init(allocator, relationship_store);
  return relationship_store;
}

void RelationshipStore_deinit(RelationshipStore *store) {
  LSTD_ASSERT(store != NULL);
  HashTable_deinit(&store->sources_for_entity);
  HashTable_deinit(&store->targets_for_entity);
}

void RelationshipStore_destroy(Allocator *allocator, RelationshipStore *store) {
  LSTD_ASSERT(allocator != NULL);
  LSTD_ASSERT(store != NULL);
  RelationshipStore_deinit(store);
  Allocator_free(allocator, store);
}

void RelationshipStore_dctor(Allocator *allocator, void *store) {
  LSTD_ASSERT(allocator != NULL);
  LSTD_ASSERT(store != NULL);
  RelationshipStore_destroy(allocator, store);
}

void ecs_insert_relationship_(Ecs *ecs, EcsId source, char *relationship_name,
                              EcsId target) {
  LSTD_ASSERT(ecs != NULL);
  LSTD_ASSERT(relationship_name != NULL);

  if (!HashTable_has(&ecs->relationship_stores, relationship_name)) {
    RelationshipStore *relationship_store =
        RelationshipStore_create(ecs->allocator);
    if (!relationship_store) {
      PANIC("Couldn't allocate relationship store");
    }

    char *owned_relationship_name =
        memory_clone_string(ecs->allocator, relationship_name);
    HashTable_insert(&ecs->relationship_stores, owned_relationship_name,
                     relationship_store);
  }

  RelationshipStore *relationship_store =
      HashTable_get(&ecs->relationship_stores, relationship_name);
  if (!relationship_store) {
    PANIC("Relationship store not found");
  }

  if (!HashTable_has(&relationship_store->sources_for_entity, &target)) {
    EcsId *key = Allocator_allocate(ecs->allocator, sizeof(EcsId));
    *key = target;
    HashSet *hash_set = Allocator_allocate(ecs->allocator, sizeof(HashSet));
    HashSet_init_with_dctor(ecs->allocator, hash_set, 16, ecs_id_hash_fn,
                            ecs_id_eq_fn, ecs_id_dctor_fn);

    HashTable_insert(&relationship_store->sources_for_entity, key, hash_set);
  }
  HashSet *sources =
      HashTable_get(&relationship_store->sources_for_entity, &target);

  if (!HashTable_has(&relationship_store->targets_for_entity, &source)) {
    EcsId *key = Allocator_allocate(ecs->allocator, sizeof(EcsId));
    *key = source;
    HashSet *hash_set = Allocator_allocate(ecs->allocator, sizeof(HashSet));
    HashSet_init_with_dctor(ecs->allocator, hash_set, 16, ecs_id_hash_fn,
                            ecs_id_eq_fn, ecs_id_dctor_fn);
    HashTable_insert(&relationship_store->targets_for_entity, key, hash_set);
  }
  HashSet *targets =
      HashTable_get(&relationship_store->targets_for_entity, &source);

  EcsId *owned_source = Allocator_allocate(ecs->allocator, sizeof(EcsId));
  *owned_source = source;
  EcsId *owned_target = Allocator_allocate(ecs->allocator, sizeof(EcsId));
  *owned_target = target;

  HashSet_insert(sources, owned_source);
  HashSet_insert(targets, owned_target);
}

bool ecs_has_component_(const Ecs *ecs, EcsId entity_id,
                        const char *component_name) {
  LSTD_ASSERT(ecs != NULL);

  ComponentStore *store = HashTable_get(&ecs->component_stores, component_name);
  if (!store)
    return false;

  return component_store_get(store, entity_id) != NULL;
}

void *ecs_get_component_(const Ecs *ecs, EcsId entity_id,
                         const char *component_name) {
  LSTD_ASSERT(ecs != NULL);

  ComponentStore *store = HashTable_get(&ecs->component_stores, component_name);

  if (!store)
    return NULL;

  return component_store_get(store, entity_id);
}
HashSet *ecs_get_relationship_sources_(const Ecs *ecs,
                                       const char *relationship_name,
                                       EcsId target) {
  LSTD_ASSERT(ecs != NULL);
  LSTD_ASSERT(relationship_name != NULL);

  RelationshipStore *store =
      HashTable_get(&ecs->relationship_stores, relationship_name);
  if (!store) {
    return NULL;
  }

  return HashTable_get(&store->sources_for_entity, &target);
}

HashSet *ecs_get_relationship_targets_(const Ecs *ecs, EcsId source,
                                       const char *relationship_name) {
  LSTD_ASSERT(ecs != NULL);
  LSTD_ASSERT(relationship_name != NULL);

  RelationshipStore *store =
      HashTable_get(&ecs->relationship_stores, relationship_name);
  if (!store) {
    return NULL;
  }
  return HashTable_get(&store->targets_for_entity, &source);
}

size_t ecs_count_matching(const Ecs *ecs, const EcsQuery *query) {
  LSTD_ASSERT(ecs != NULL);
  LSTD_ASSERT(query != NULL);

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
  LSTD_ASSERT(ecs != NULL);
  LSTD_ASSERT(query != NULL);

  bool matches = true;
  size_t component_index = 0;
  while (component_index < query->component_count &&
         query->components[component_index] != NULL) {
    ComponentStore *component_store = HashTable_get(
        &ecs->component_stores, query->components[component_index]);

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
  LSTD_ASSERT(ecs != NULL);
  LSTD_ASSERT(query != NULL);

  EcsQueryIt iterator = {0};
  iterator.allocator = ecs->allocator;
  iterator.state = Allocator_allocate(ecs->allocator, sizeof(EcsQueryItState));
  LSTD_ASSERT(iterator.state != NULL);
  memset(iterator.state, 0, sizeof(EcsQueryItState));
  iterator.state->iterating = false;
  iterator.state->matching_entities = Allocator_allocate_array(
      ecs->allocator, ecs->entity_count, sizeof(size_t));
  LSTD_ASSERT(iterator.state->matching_entities != NULL);

  for (size_t i = 0; i < query->component_count; i++) {
    LSTD_ASSERT(i < ECS_QUERY_MAX_COMPONENT_COUNT - 1);
    ComponentStore *component_store =
        HashTable_get(&ecs->component_stores, query->components[i]);
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
  LSTD_ASSERT(it != NULL);
  LSTD_ASSERT(it->state != NULL);

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
  LSTD_ASSERT(it != NULL);
  if (it->state == NULL)
    return;
  Allocator_free(it->allocator, it->state->matching_entities);
  Allocator_free(it->allocator, it->state);
  it->state = NULL;
  it->allocator = NULL;
}
void *ecs_query_it_get_(const EcsQueryIt *it, size_t component) {
  LSTD_ASSERT(it != NULL);
  LSTD_ASSERT(it->state != NULL);

  if (!it->state->component_stores[component])
    return NULL;

  return component_store_get(
      it->state->component_stores[component],
      it->state->matching_entities[it->state->current_matching_entity]);
}
EcsId ecs_query_it_entity_id(const EcsQueryIt *it) {
  LSTD_ASSERT(it != NULL);
  return it->state->matching_entities[it->state->current_matching_entity];
}
