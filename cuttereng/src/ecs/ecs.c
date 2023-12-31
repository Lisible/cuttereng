#include "ecs.h"
#include "../assert.h"
#include "../bitset.h"
#include "../log.h"
#include "../memory.h"
#include <string.h>

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
DEF_HASH_TABLE(ComponentStore, HashTableComponentStore, component_store_destroy)

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

void ecs_init(Allocator *allocator, Ecs *ecs) {
  ASSERT(ecs != NULL);

  ecs->allocator = allocator;
  ecs->entity_count = 0;
  ecs->component_stores = HashTableComponentStore_create(allocator, 16);
}
void ecs_deinit(Ecs *ecs) {
  HashTableComponentStore_destroy(ecs->component_stores);
}

EcsId ecs_create_entity(Ecs *ecs) {
  ASSERT(ecs != NULL);

  EcsId id = ecs->entity_count;
  ecs->entity_count++;
  return id;
}

size_t ecs_get_entity_count(const Ecs *ecs) {
  ASSERT(ecs != NULL);
  return ecs->entity_count;
}

void ecs_insert_component_(Ecs *ecs, EcsId entity_id, char *component_name,
                           size_t component_size, const void *data) {
  ASSERT(ecs != NULL);
  ASSERT(data != NULL);

  if (!HashTableComponentStore_has(ecs->component_stores, component_name)) {
    ComponentStore *store = component_store_new(ecs->allocator, component_size);
    if (!store) {
      goto err;
    }
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

  // FIXME this is O(n + m) with n being the entity count and m being the query
  // component count. Using a hierarchical bitset could help.
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
  while (query->components[component_index] != NULL) {
    ComponentStore *component_store = HashTableComponentStore_get(
        ecs->component_stores, query->components[component_index]);
    matches = matches && BITTEST(component_store->bitset, entity_id);
    component_index++;
  }

  return matches;
}

#define ECS_QUERY_MAX_COMPONENT_COUNT 16
struct EcsQueryItState {
  ComponentStore *component_stores[ECS_QUERY_MAX_COMPONENT_COUNT];
  EcsId *matching_entities;
  size_t current_matching_entity;
  size_t matching_entity_count;
};

EcsQueryIt ecs_query(const Ecs *ecs, const EcsQuery *query) {
  ASSERT(ecs != NULL);
  ASSERT(query != NULL);

  EcsQueryIt iterator = {0};
  iterator.allocator = ecs->allocator;
  iterator.state = allocator_allocate(ecs->allocator, sizeof(EcsQueryItState));
  ASSERT(iterator.state != NULL);

  memset(iterator.state, 0, sizeof(EcsQueryItState));

  iterator.state->matching_entities = allocator_allocate_array(
      ecs->allocator, ecs->entity_count, sizeof(size_t));
  ASSERT(iterator.state->matching_entities != NULL);

  // TODO handle the case where the query has more than
  // ECS_QUERY_MAX_COMPONENT_COUNT components
  size_t i = 0;
  while (query->components[i]) {
    ASSERT(i < ECS_QUERY_MAX_COMPONENT_COUNT - 1,
           "Too many components in the query");
    ComponentStore *component_store = HashTableComponentStore_get(
        ecs->component_stores, query->components[i]);
    iterator.state->component_stores[i] = component_store;
    i++;
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

  if (it->state->current_matching_entity >= it->state->matching_entity_count) {
    ecs_query_it_deinit(it);
    return false;
  }

  it->state->current_matching_entity++;

  return true;
}
void ecs_query_it_deinit(EcsQueryIt *it) {
  ASSERT(it != NULL);
  allocator_free(it->allocator, it->state->matching_entities);
  allocator_free(it->allocator, it->state);
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
