#include "ecs.h"
#include "common.h"
#include "src/bitset.h"
#include "src/hash.h"
#include "src/memory.h"
#include <string.h>

typedef struct component_store {
  unsigned int *bitset;
  void *data;
  size_t length;
  size_t capacity;
  size_t item_size;
} component_store;

component_store *component_store_new(size_t item_size) {
  const size_t INITIAL_CAPACITY = 64;
  component_store *store = memory_allocate(sizeof(component_store));
  store->bitset = memory_allocate_array(100000 / 8 + 1, sizeof(unsigned int));
  store->capacity = INITIAL_CAPACITY;
  store->length = 0;
  store->item_size = item_size;
  store->data = memory_allocate(store->capacity * store->item_size);
  return store;
}

void component_store_destroy(component_store *store) {
  memory_free(store->bitset);
  memory_free(store->data);
  memory_free(store);
}

void component_store_set(component_store *store, ecs_id entity_id, void *data) {
  char *dst = (char *)store->data;
  memcpy(&dst[entity_id * store->item_size], (char *)data, store->item_size);
  bitset_set(store->bitset, entity_id);
}

void ecs_init(ecs *ecs) {
  ecs->entity_count = 0;
  ecs->component_stores = hash_table_new(NULL);
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

  return bitset_is_set(store->bitset, entity_id);
}
