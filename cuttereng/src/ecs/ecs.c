#include "ecs.h"
#include "../assert.h"
#include "../bitset.h"
#include "../log.h"
#include "../memory.h"
#include "src/vec.h"
#include <string.h>

DEF_VEC(EcsSystem, EcsSystemVec, 512)
DEF_VEC(EcsCommand, EcsCommandVec, 128)

struct ComponentStore {
  void *data;
  size_t length;
  size_t capacity;
  size_t item_size;
  // TODO make a growable bitset, maybe hierarchical at some point
  uint64_t bitset[8];
};

ComponentStore *component_store_new(Allocator *allocator, size_t item_size) {
  const size_t INITIAL_CAPACITY = 64;
  ComponentStore *store = allocator_allocate(allocator, sizeof(ComponentStore));
  if (!store)
    goto err;

  store->capacity = INITIAL_CAPACITY;
  store->length = 0;
  store->item_size = item_size;
  memset(store->bitset, 0, 8);
  store->data =
      allocator_allocate_array(allocator, store->capacity, store->item_size);
  if (!store->data)
    goto err;

  return store;

err:
  LOG_ERROR("ComponentStore allocation failed");
  return NULL;
}

void component_store_destroy(Allocator *allocator, ComponentStore *store) {
  ASSERT(store != NULL);

  allocator_free(allocator, store->data);
  allocator_free(allocator, store);
}
DEF_HASH_TABLE(ComponentStore *, HashTableComponentStore,
               component_store_destroy)

void component_store_set(ComponentStore *store, EcsId entity_id,
                         const void *data) {
  ASSERT(store != NULL);
  ASSERT(data != NULL);

  char *dst = (char *)store->data;
  memcpy(dst + (entity_id * store->item_size), (char *)data,
         store->item_size * sizeof(char));
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
void ecs_command_execute(Ecs *ecs, EcsCommand *command) {
  switch (command->type) {
  case EcsCommandType_RegisterSystem:
    ecs_register_system(ecs, &command->register_system.system_descriptor);
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
  case EcsCommandType_RegisterSystem: {
    size_t component_count =
        command->register_system.system_descriptor.query.component_count;
    for (size_t i = 0; i < component_count; i++) {
      allocator_free(
          allocator,
          command->register_system.system_descriptor.query.components[i]);
    }
  } break;
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
  command.register_system.system_descriptor.fn = system->fn;
  command.register_system.system_descriptor.query.component_count =
      system->query.component_count;
  for (size_t i = 0; i < system->query.component_count; i++) {
    char *component_id =
        memory_clone_string(queue->allocator, system->query.components[i]);
    command.register_system.system_descriptor.query.components[i] =
        component_id;
  }

  EcsCommandVec_append(&queue->commands, &command, 1);
}
EcsId ecs_command_queue_create_entity(EcsCommandQueue *queue) {
  ASSERT(queue != NULL);
  EcsCommand command = {.type = EcsCommandType_CreateEntity};
  EcsId entity_id = ecs_reserve_entity(queue->ecs);
  EcsCommandVec_append(&queue->commands, &command, 1);
  return entity_id;
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

void ecs_default_init_system(EcsCommandQueue *queue) { (void)queue; }
void ecs_init(Allocator *allocator, Ecs *ecs, EcsInitSystem init_system) {
  ASSERT(allocator != NULL);
  ASSERT(ecs != NULL);

  ecs->allocator = allocator;
  ecs->entity_count = 0;
  ecs->reserved_entity_count = 0;
  ecs->component_stores = HashTableComponentStore_create(allocator, 16);
  EcsSystemVec_init(allocator, &ecs->systems);
  ecs_command_queue_init(allocator, &ecs->command_queue, ecs);
  init_system(&ecs->command_queue);
}
void ecs_deinit(Ecs *ecs) {
  ecs_command_queue_deinit(&ecs->command_queue);
  EcsSystemVec_deinit(&ecs->systems);
  HashTableComponentStore_destroy(ecs->component_stores);
}
void ecs_register_system(Ecs *ecs,
                         const EcsSystemDescriptor *system_descriptor) {
  EcsSystemVec_append(&ecs->systems,
                      &(const EcsSystem){.query = system_descriptor->query,
                                         .fn = system_descriptor->fn},
                      1);
}

void ecs_run_systems(Ecs *ecs) {
  ASSERT(ecs != NULL);
  for (size_t i = 0; i < ecs->systems.length; i++) {
    EcsQueryIt it = ecs_query(ecs, &ecs->systems.data[i].query);
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
  ASSERT(data != NULL);

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
    ASSERT(component_store != NULL);
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
