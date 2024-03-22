#ifndef CUTTERENG_HASH_H
#define CUTTERENG_HASH_H

#include "assert.h"
#include "memory.h"
#include <stdbool.h>
#include <string.h>

void HashTable_noop_destructor(Allocator *allocator, void *value);
#define DECL_HASH_TABLE(K, V, name)                                            \
  struct name##KV {                                                            \
    K key;                                                                     \
    V value;                                                                   \
    bool is_present;                                                           \
  };                                                                           \
  typedef struct name##KV name##KV;                                            \
  struct name {                                                                \
    name##KV *items;                                                           \
    size_t capacity;                                                           \
    size_t length;                                                             \
    Allocator *allocator;                                                      \
  };                                                                           \
  typedef struct name name;                                                    \
  name *name##_create(Allocator *allocator, size_t initial_capacity);          \
  void name##_destroy(name *table);                                            \
  char *name##_set_strkey(name *table, char *key, V value);                    \
  char *name##_set(name *table, char *key, size_t key_size, V value);          \
  V name##_get(const name *table, const K key, size_t key_size);               \
  bool name##_has(const name *table, const K key, size_t key_size);            \
  size_t name##_length(const name *table);                                     \
  void name##_remove(name *table, const K key, size_t key_size);               \
  void name##_steal(name *table, const K key, size_t key_size);                \
  void name##_clear(name *table);                                              \
  bool name##_expand(name *table);

#define DEF_HASH_TABLE(K, V, name, item_dctor_fn)                              \
  name *name##_create(Allocator *allocator, size_t initial_capacity) {         \
    name *table = allocator_allocate(allocator, sizeof(name));                 \
    table->allocator = allocator;                                              \
    table->items = allocator_allocate_array(allocator, initial_capacity,       \
                                            sizeof(name##KV));                 \
    table->capacity = initial_capacity;                                        \
    table->length = 0;                                                         \
    return table;                                                              \
  }                                                                            \
  void name##_destroy(name *table) {                                           \
    ASSERT(table != NULL);                                                     \
    name##_clear(table);                                                       \
    allocator_free(table->allocator, table->items);                            \
    allocator_free(table->allocator, table);                                   \
  }                                                                            \
  size_t name##_index_for_key(name *table, const K key, size_t key_size) {     \
    ASSERT(table != NULL);                                                     \
    ASSERT(key != NULL);                                                       \
    uint64_t hash = hash_fnv_1a(key, key_size);                                \
    return hash & (table->capacity - 1);                                       \
  }                                                                            \
  char *name##_set_strkey(name *table, char *key, V value) {                   \
    ASSERT(table != NULL);                                                     \
    ASSERT(key != NULL);                                                       \
    char *cloned_key = memory_clone_string(table->allocator, key);             \
    if (!cloned_key) {                                                         \
      PANIC("Couldn't allocate hash table key");                               \
    }                                                                          \
    return name##_set(table, cloned_key, strlen(cloned_key), value);           \
  }                                                                            \
  char *name##_set(name *table, K key, size_t key_size, V value) {             \
    ASSERT(table != NULL);                                                     \
    ASSERT(key != NULL);                                                       \
    ASSERT(value != NULL);                                                     \
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
    size_t index = name##_index_for_key(table, key, key_size);                 \
    while (table->items[index].is_present) {                                   \
      if (memcmp(key, table->items[index].key, key_size) == 0) {               \
        table->items[index].value = value;                                     \
        return table->items[index].key;                                        \
      }                                                                        \
                                                                               \
      index++;                                                                 \
      if (index >= table->capacity) {                                          \
        index = 0;                                                             \
      }                                                                        \
    }                                                                          \
    table->length++;                                                           \
    table->items[index].key = key;                                             \
    table->items[index].value = value;                                         \
    table->items[index].is_present = true;                                     \
    return key;                                                                \
  }                                                                            \
  V name##_get(const name *table, const K key, size_t key_size) {              \
    ASSERT(table != NULL);                                                     \
    ASSERT(key != NULL);                                                       \
    if (!key)                                                                  \
      return NULL;                                                             \
                                                                               \
    uint64_t hash = hash_fnv_1a(key, key_size);                                \
    size_t index = hash & (table->capacity - 1);                               \
    while (table->items[index].is_present) {                                   \
      if (memcmp(key, table->items[index].key, key_size) == 0) {               \
        return table->items[index].value;                                      \
      }                                                                        \
                                                                               \
      index++;                                                                 \
    }                                                                          \
    return NULL;                                                               \
  }                                                                            \
  bool name##_has(const name *table, const K key, size_t key_size) {           \
    ASSERT(table != NULL);                                                     \
    ASSERT(key != NULL);                                                       \
    if (!key)                                                                  \
      return false;                                                            \
    uint64_t hash = hash_fnv_1a(key, key_size);                                \
    size_t index = hash & (table->capacity - 1);                               \
    while (table->items[index].is_present) {                                   \
      if (memcmp(key, table->items[index].key, key_size) == 0) {               \
        return true;                                                           \
      }                                                                        \
                                                                               \
      index++;                                                                 \
    }                                                                          \
                                                                               \
    return false;                                                              \
  }                                                                            \
  size_t name##_length(const name *table) {                                    \
    ASSERT(table != NULL);                                                     \
    return table->length;                                                      \
  }                                                                            \
  void name##_remove_at_index(name *table, size_t index,                       \
                              bool execute_item_dctor) {                       \
    ASSERT(table != NULL);                                                     \
    if (!table->items[index].is_present) {                                     \
      return;                                                                  \
    }                                                                          \
    if (execute_item_dctor) {                                                  \
      item_dctor_fn(table->allocator, table->items[index].value);              \
    }                                                                          \
    allocator_free(table->allocator, table->items[index].key);                 \
    table->items[index].is_present = false;                                    \
    table->length--;                                                           \
  }                                                                            \
                                                                               \
  void name##_remove(name *table, const K key, size_t key_size) {              \
    ASSERT(table != NULL);                                                     \
    ASSERT(key != NULL);                                                       \
    size_t index = name##_index_for_key(table, key, key_size);                 \
    name##_remove_at_index(table, index, true);                                \
  }                                                                            \
  void name##_steal(name *table, const K key, size_t key_size) {               \
    ASSERT(table != NULL);                                                     \
    ASSERT(key != NULL);                                                       \
    size_t index = name##_index_for_key(table, key, key_size);                 \
    name##_remove_at_index(table, index, false);                               \
  }                                                                            \
  void name##_clear(name *table) {                                             \
    ASSERT(table != NULL);                                                     \
    for (size_t i = 0; i < table->capacity; i++) {                             \
      if (table->items[i].is_present) {                                        \
        name##_remove_at_index(table, i, true);                                \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  bool name##_expand(name *table) {                                            \
    ASSERT(table != NULL);                                                     \
    size_t new_capacity = table->capacity * 2;                                 \
    if (new_capacity < table->capacity) {                                      \
      return false;                                                            \
    }                                                                          \
    name##KV *new_items = allocator_allocate_array(                            \
        table->allocator, new_capacity, sizeof(name##KV));                     \
    if (new_items == NULL) {                                                   \
      return false;                                                            \
    }                                                                          \
    for (size_t i = 0; i < table->capacity; i++) {                             \
      name##KV kv = table->items[i];                                           \
      if (kv.key != NULL) {                                                    \
        uint64_t hash = hash_fnv_1a(kv.key, strlen(kv.key));                   \
        size_t index = hash & (new_capacity - 1);                              \
        bool found = false;                                                    \
        while (new_items[index].is_present) {                                  \
          if (strcmp(kv.key, new_items[index].key) == 0) {                     \
            new_items[index].value = kv.value;                                 \
            found = true;                                                      \
            break;                                                             \
          }                                                                    \
          index++;                                                             \
          if (index >= new_capacity) {                                         \
            index = 0;                                                         \
          }                                                                    \
        }                                                                      \
        if (!found) {                                                          \
          new_items[index].key = kv.key;                                       \
          new_items[index].value = kv.value;                                   \
          new_items[index].is_present = true;                                  \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    allocator_free(table->allocator, table->items);                            \
    table->capacity = new_capacity;                                            \
    table->items = new_items;                                                  \
    return true;                                                               \
  }

uint64_t hash_fnv_1a(const char *bytes, size_t nbytes);

#endif // CUTTERENG_HASH_H
