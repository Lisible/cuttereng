#ifndef CUTTERENG_HASH_H
#define CUTTERENG_HASH_H

#include "common.h"
#include "memory.h"
#include <stdbool.h>
#include <string.h>

void HashTable_noop_destructor(void *value);
#define DECL_HASH_TABLE(V, name)                                               \
  typedef struct {                                                             \
    char *key;                                                                 \
    V *value;                                                                  \
  } name##KV;                                                                  \
  typedef struct {                                                             \
    name##KV *items;                                                           \
    size_t capacity;                                                           \
    size_t length;                                                             \
  } name;                                                                      \
  name *name##_create(size_t initial_capacity);                                \
  void name##_destroy(name *table);                                            \
  char *name##_set(name *table, char *key, V *value);                          \
  V *name##_get(const name *table, const char *key);                           \
  bool name##_has(const name *table, const char *key);                         \
  size_t name##_length(const name *table);                                     \
  void name##_remove(name *table, const char *key);                            \
  void name##_steal(name *table, const char *key);                             \
  void name##_clear(name *table);                                              \
  bool name##_expand(name *table);

#define DEF_HASH_TABLE(V, name, item_dctor_fn)                                 \
  name *name##_create(size_t initial_capacity) {                               \
    name *table = memory_allocate(sizeof(name));                               \
    table->items = memory_allocate_array(initial_capacity, sizeof(name##KV));  \
    table->capacity = initial_capacity;                                        \
    table->length = 0;                                                         \
    return table;                                                              \
  }                                                                            \
  void name##_destroy(name *table) {                                           \
    ASSERT(table != NULL);                                                     \
    memory_free(table->items);                                                 \
    memory_free(table);                                                        \
  }                                                                            \
  size_t name##_index_for_key(name *table, const char *key) {                  \
    uint64_t hash = hash_fnv_1a(key);                                          \
    return hash & (table->capacity - 1);                                       \
  }                                                                            \
  char *name##_set(name *table, char *key, V *value) {                         \
    ASSERT(table != NULL);                                                     \
    if (value == NULL) {                                                       \
      return NULL;                                                             \
    }                                                                          \
                                                                               \
    if (table->length >= table->capacity / 2) {                                \
      if (!name##_expand(table)) {                                             \
        return NULL;                                                           \
      }                                                                        \
    }                                                                          \
                                                                               \
    size_t index = name##_index_for_key(table, key);                           \
    while (table->items[index].key != NULL) {                                  \
      if (strcmp(key, table->items[index].key) == 0) {                         \
        table->items[index].value = value;                                     \
        return table->items[index].key;                                        \
      }                                                                        \
                                                                               \
      index++;                                                                 \
      if (index >= table->capacity) {                                          \
        index = 0;                                                             \
      }                                                                        \
    }                                                                          \
                                                                               \
    key = strdup(key);                                                         \
    if (!key)                                                                  \
      return NULL;                                                             \
    table->length++;                                                           \
    table->items[index].key = key;                                             \
    table->items[index].value = value;                                         \
    return key;                                                                \
  }                                                                            \
  V *name##_get(const name *table, const char *key) {                          \
    ASSERT(table != NULL);                                                     \
    if (!key)                                                                  \
      return NULL;                                                             \
                                                                               \
    uint64_t hash = hash_fnv_1a(key);                                          \
    size_t index = hash & (table->capacity - 1);                               \
    while (table->items[index].key != NULL) {                                  \
      if (strcmp(key, table->items[index].key) == 0) {                         \
        return table->items[index].value;                                      \
      }                                                                        \
                                                                               \
      index++;                                                                 \
    }                                                                          \
                                                                               \
    return NULL;                                                               \
  }                                                                            \
  bool name##_has(const name *table, const char *key) {                        \
    ASSERT(table != NULL);                                                     \
    if (!key)                                                                  \
      return false;                                                            \
    uint64_t hash = hash_fnv_1a(key);                                          \
    size_t index = hash & (table->capacity - 1);                               \
    while (table->items[index].key != NULL) {                                  \
      if (strcmp(key, table->items[index].key) == 0) {                         \
        return true;                                                           \
      }                                                                        \
                                                                               \
      index++;                                                                 \
    }                                                                          \
                                                                               \
    return false;                                                              \
  }                                                                            \
  size_t name##_length(const name *table) { return table->length; }            \
  void name##_remove_at_index(name *table, size_t index,                       \
                              bool execute_item_dctor) {                       \
    if (!table->items[index].key) {                                            \
      return;                                                                  \
    }                                                                          \
    if (execute_item_dctor && item_dctor_fn != NULL) {                         \
      item_dctor_fn(table->items[index].value);                                \
    }                                                                          \
    memory_free(table->items[index].key);                                      \
    table->items[index].key = NULL;                                            \
    table->items[index].value = NULL;                                          \
    table->length--;                                                           \
  }                                                                            \
                                                                               \
  void name##_remove(name *table, const char *key) {                           \
    size_t index = name##_index_for_key(table, key);                           \
    name##_remove_at_index(table, index, true);                                \
  }                                                                            \
  void name##_steal(name *table, const char *key) {                            \
    size_t index = name##_index_for_key(table, key);                           \
    name##_remove_at_index(table, index, false);                               \
  }                                                                            \
  void name##_clear(name *table) {                                             \
    for (size_t i = 0; i < table->capacity; i++) {                             \
      if (table->items[i].key != NULL) {                                       \
        name##_remove_at_index(table, i, true);                                \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  bool name##_expand(name *table) {                                            \
    size_t new_capacity = table->capacity * 2;                                 \
    if (new_capacity < table->capacity) {                                      \
      return false;                                                            \
    }                                                                          \
    name##KV *new_items =                                                      \
        memory_allocate_array(new_capacity, sizeof(name##KV));                 \
    if (new_items == NULL) {                                                   \
      return false;                                                            \
    }                                                                          \
    for (size_t i = 0; i < table->capacity; i++) {                             \
      name##KV kv = table->items[i];                                           \
      if (kv.key != NULL) {                                                    \
        uint64_t hash = hash_fnv_1a(kv.key);                                   \
        size_t index = hash & (table->capacity - 1);                           \
        bool found = false;                                                    \
        while (new_items[index].key != NULL) {                                 \
          if (strcmp(kv.key, new_items[index].key) == 0) {                     \
            new_items[index].value = kv.value;                                 \
            found = true;                                                      \
            break;                                                             \
          }                                                                    \
          index++;                                                             \
          if (index >= table->capacity) {                                      \
            index = 0;                                                         \
          }                                                                    \
        }                                                                      \
        if (!found) {                                                          \
          new_items[index].key = kv.key;                                       \
          new_items[index].value = kv.value;                                   \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    memory_free(table->items);                                                 \
    table->capacity = new_capacity;                                            \
    table->items = new_items;                                                  \
    return true;                                                               \
  }

uint64_t hash_fnv_1a(const char *bytes);

#endif // CUTTERENG_HASH_H
