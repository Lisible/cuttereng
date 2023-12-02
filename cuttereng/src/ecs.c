#include "ecs.h"
#include "common.h"
#include "src/bitset.h"
#include "src/hash.h"
#include "src/memory.h"
#include <string.h>

typedef struct component_store {
  void *data;
  size_t length;
  size_t capacity;
  size_t item_size;
  // TODO make a growable bitset, maybe hierarchical at some point
  uint64_t bitset[8];
} component_store;

component_store *component_store_new(size_t item_size) {
  const size_t INITIAL_CAPACITY = 64;
  component_store *store = memory_allocate(sizeof(component_store));
  store->capacity = INITIAL_CAPACITY;
  store->length = 0;
  store->item_size = item_size;
  memset(store->bitset, 0, 8);
  store->data = memory_allocate_array(store->capacity, store->item_size);
  return store;
}

void component_store_destroy(component_store *store) {
  memory_free(store->data);
  memory_free(store);
}

void component_store_item_destructor(void *item) {
  component_store_destroy(item);
}

void component_store_set(component_store *store, ecs_id entity_id, void *data) {
  char *dst = (char *)store->data;
  memcpy(dst + (entity_id * store->item_size), (char *)data,
         store->item_size * sizeof(char));
  BITSET(store->bitset, entity_id);
}

void *component_store_get(component_store *store, ecs_id entity_id) {
  if (!BITTEST(store->bitset, entity_id)) {
    return NULL;
  }

  char *ptr = (char *)store->data;
  return ptr + (entity_id * store->item_size);
}

void ecs_init(ecs *ecs) {
  ecs->entity_count = 0;
  ecs->component_stores = hash_table_new(component_store_item_destructor);
}
void ecs_deinit(ecs *ecs) { hash_table_destroy(ecs->component_stores); }

ecs_id ecs_create_entity(ecs *ecs) {
  ecs_id id = ecs->entity_count;
  ecs->entity_count++;
  return id;
}

size_t ecs_get_entity_count(ecs *ecs) { return ecs->entity_count; }

void ecs_insert_component(ecs *ecs, ecs_id entity_id, char *component_name,
                          size_t component_size, void *data) {
  if (!hash_table_has(ecs->component_stores, component_name)) {
    component_store *store = component_store_new(component_size);
    hash_table_set(ecs->component_stores, component_name, store);
  }

  component_store *store =
      hash_table_get(ecs->component_stores, component_name);
  component_store_set(store, entity_id, data);
}

bool ecs_has_component(ecs *ecs, ecs_id entity_id, char *component_name) {
  component_store *store =
      hash_table_get(ecs->component_stores, component_name);
  if (!store)
    return false;

  return component_store_get(store, entity_id) != NULL;
}

void *ecs_get_component(ecs *ecs, ecs_id entity_id, char *component_name) {
  component_store *store =
      hash_table_get(ecs->component_stores, component_name);

  if (!store)
    return NULL;

  return component_store_get(store, entity_id);
}

size_t ecs_count_matching(ecs *ecs, ecs_query *query) {
  size_t result = 0;

  // FIXME this is O(n + m) with n being the entity count
  // Using a hierarchical bitset could help
  for (size_t entity_id = 0; entity_id < ecs_get_entity_count(ecs);
       entity_id++) {
    bool matches = true;
    size_t component_index = 0;
    while (query->components[component_index] != NULL) {
      component_store *component_store = hash_table_get(
          ecs->component_stores, query->components[component_index]);
      matches = matches && BITTEST(component_store->bitset, entity_id);
      component_index++;
    }

    if (matches)
      result++;
  }

  return result;
}
