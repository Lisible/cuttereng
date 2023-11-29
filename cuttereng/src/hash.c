#include "hash.h"
#include "src/log.h"
#include "src/memory.h"
#include <stddef.h>
#include <string.h>

struct hash_table {
  hash_table_kv *items;
  size_t capacity;
  size_t length;
  item_destructor_fn item_destructor;
};

size_t hash_table_index_for_key(hash_table *table, const char *key);
hash_table *hash_table_new(item_destructor_fn item_destructor) {
  static const size_t INITIAL_CAPACITY = 16;
  hash_table *table = memory_allocate(sizeof(hash_table));
  table->capacity = INITIAL_CAPACITY;
  table->length = 0;
  table->item_destructor = item_destructor;
  if (table->item_destructor == NULL) {
    table->item_destructor = memory_free;
  }
  table->items = memory_allocate_array(table->capacity, sizeof(hash_table_kv));
  return table;
}

void hash_table_destroy(hash_table *table) {
  for (size_t i = 0; i < table->capacity; i++) {
    memory_free((void *)table->items[i].key);

    if (table->items[i].value != NULL)
      table->item_destructor(table->items[i].value);
  }

  memory_free(table->items);
  memory_free(table);
}

const char *hash_table_set_kv(hash_table_kv *items, size_t capacity,
                              const char *key, void *value, size_t *length) {

  uint64_t hash = hash_fnv_1a(key);
  // Note: capacity is a power of two so we can get the modulo using &
  size_t index = hash & (capacity - 1);
  while (items[index].key != NULL) {
    if (strcmp(key, items[index].key) == 0) {
      items[index].value = value;
      return items[index].key;
    }

    index++;
    if (index >= capacity) {
      index = 0;
    }
  }

  if (length != NULL) {
    key = strdup(key);
    if (!key)
      return NULL;

    (*length)++;
  }
  items[index].key = key;
  items[index].value = value;
  return key;
}

bool hash_table_expand(hash_table *table) {
  size_t new_capacity = table->capacity * 2;
  if (new_capacity < table->capacity) {
    return false;
  }

  hash_table_kv *new_items =
      memory_allocate_array(new_capacity, sizeof(hash_table_kv));
  if (new_items == NULL) {
    return false;
  }

  for (size_t i = 0; i < table->capacity; i++) {
    hash_table_kv kv = table->items[i];
    if (kv.key != NULL) {
      hash_table_set_kv(table->items, table->capacity, kv.key, kv.value, NULL);
    }
  }
  memory_free(table->items);
  table->capacity = new_capacity;
  table->items = new_items;
  return true;
}

const char *hash_table_set(hash_table *table, const char *key, void *value) {
  if (value == NULL)
    return NULL;

  if (table->length >= table->capacity / 2) {
    if (!hash_table_expand(table)) {
      return NULL;
    }
  }

  return hash_table_set_kv(table->items, table->capacity, key, value,
                           &table->length);
}

void *hash_table_get(hash_table *table, const char *key) {
  if (!key)
    return NULL;

  uint64_t hash = hash_fnv_1a(key);
  size_t index = hash & (table->capacity - 1);
  while (table->items[index].key != NULL) {
    if (strcmp(key, table->items[index].key) == 0) {
      return table->items[index].value;
    }

    index++;
  }

  return NULL;
}
bool hash_table_has(hash_table *table, const char *key) {
  if (key == NULL)
    return false;

  uint64_t hash = hash_fnv_1a(key);
  size_t index = hash & (table->capacity - 1);
  while (table->items[index].key != NULL) {
    if (strcmp(key, table->items[index].key) == 0) {
      return true;
    }

    index++;
  }

  return false;
}

void hash_table_remove_at_index(hash_table *table, size_t index) {
  if (!table->items[index].key) {
    return;
  }

  memory_free((void *)table->items[index].key);
  table->items[index].key = NULL;
  table->items[index].value = NULL;
  table->length--;
}

void hash_table_steal(hash_table *table, const char *key) {
  size_t index = hash_table_index_for_key(table, key);
  hash_table_remove_at_index(table, index);
}

size_t hash_table_index_for_key(hash_table *table, const char *key) {
  uint64_t hash = hash_fnv_1a(key);
  return hash & (table->capacity - 1);
}

size_t hash_table_length(hash_table *hash_table) { return hash_table->length; }

uint64_t hash_fnv_1a(const char *bytes) {
  static const uint64_t FNV_OFFSET_BASIS = 14695981039346656037u;
  static const uint64_t FNV_PRIME = 1099511628211u;

  uint64_t hash = FNV_OFFSET_BASIS;
  size_t i = 0;
  while (bytes[i] != 0) {
    hash = hash ^ bytes[i];
    hash = hash * FNV_PRIME;
    i++;
  }

  return hash;
}
