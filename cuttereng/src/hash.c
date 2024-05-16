#include "hash.h"
#include "assert.h"
#include "memory.h"
#include <stddef.h>
#include <string.h>

void HashTable_noop_dctor_fn(Allocator *allocator, void *v) {
  (void)allocator;
  (void)v;
}
bool HashTable_init(Allocator *allocator, HashTable *hash_table,
                    const size_t initial_capacity,
                    HashTableKeyHashFn key_hash_fn,
                    HashTableKeyEqFn key_eq_fn) {
  ASSERT(allocator != NULL);
  ASSERT(hash_table != NULL);
  hash_table->items = allocator_allocate_array(allocator, initial_capacity,
                                               sizeof(HashTableKV));
  if (!hash_table->items) {
    return false;
  }

  hash_table->allocator = allocator;
  hash_table->capacity = initial_capacity;
  hash_table->length = 0;
  hash_table->key_hash_fn = key_hash_fn;
  hash_table->key_eq_fn = key_eq_fn;
  hash_table->key_dctor_fn = HashTable_noop_dctor_fn;
  hash_table->value_dctor_fn = HashTable_noop_dctor_fn;
  return true;
}

bool HashTable_init_with_dctors(Allocator *allocator, HashTable *hash_table,
                                const size_t initial_capacity,
                                HashTableKeyHashFn key_hash_fn,
                                HashTableKeyEqFn key_eq_fn,
                                HashTableDctorFn key_dctor_fn,
                                HashTableDctorFn value_dctor_fn) {
  ASSERT(hash_table != NULL);
  if (!HashTable_init(allocator, hash_table, initial_capacity, key_hash_fn,
                      key_eq_fn)) {
    return false;
  }
  hash_table->key_dctor_fn = key_dctor_fn;
  hash_table->value_dctor_fn = value_dctor_fn;
  return true;
}

void HashTable_deinit(HashTable *hash_table) {
  ASSERT(hash_table != NULL);
  HashTable_clear(hash_table);
  allocator_free(hash_table->allocator, hash_table->items);
}

static size_t HashTable_index_for_key(const HashTable *hash_table,
                                      const void *key) {
  ASSERT(hash_table != NULL);
  ASSERT(key != NULL);
  uint64_t hashed_key = hash_table->key_hash_fn(key);
  return hashed_key % hash_table->capacity;
}

bool HashTable_expand(HashTable *hash_table) {
  ASSERT(hash_table != NULL);
  size_t new_capacity = hash_table->capacity * 2;

  // Checking for eventual overflow
  if (new_capacity < hash_table->capacity) {
    return false;
  }

  HashTableKV *new_items = allocator_allocate_array(
      hash_table->allocator, new_capacity, sizeof(HashTableKV));
  if (!new_items) {
    return false;
  }

  for (size_t i = 0; i < hash_table->capacity; i++) {
    HashTableKV *item = &hash_table->items[i];
    if (item->is_present) {
      uint64_t key_hash = hash_table->key_hash_fn(item->key);
      size_t new_index = key_hash % new_capacity;
      bool found = false;
      while (new_items[new_index].is_present) {
        if (hash_table->key_eq_fn(new_items[new_index].key, item->key)) {
          new_items[new_index].value = item->value;
          found = true;
          break;
        }

        new_index++;
        if (new_index >= new_capacity) {
          new_index = 0;
        }
      }

      if (!found) {
        new_items[new_index].key = item->key;
        new_items[new_index].value = item->value;
        new_items[new_index].is_present = true;
      }
    }
  }

  allocator_free(hash_table->allocator, hash_table->items);
  hash_table->items = new_items;
  hash_table->capacity = new_capacity;
  return true;
}

bool HashTable_insert(HashTable *hash_table, void *key, void *value) {
  ASSERT(hash_table != NULL);
  ASSERT(key != NULL);

  if (hash_table->length >= hash_table->capacity / 2) {
    if (!HashTable_expand(hash_table)) {
      return false;
    }
  }

  size_t index = HashTable_index_for_key(hash_table, key);
  while (hash_table->items[index].is_present) {
    if (hash_table->key_eq_fn(hash_table->items[index].key, key)) {
      hash_table->items[index].value = value;
      return true;
    }
    index++;
    if (index >= hash_table->capacity) {
      index = 0;
    }
  }

  hash_table->items[index].key = key;
  hash_table->items[index].value = value;
  hash_table->items[index].is_present = true;
  hash_table->length++;
  return true;
}

void *HashTable_get(const HashTable *hash_table, const void *key) {
  ASSERT(hash_table != NULL);
  ASSERT(key != NULL);
  size_t index = HashTable_index_for_key(hash_table, key);
  while (hash_table->items[index].is_present) {
    if (hash_table->key_eq_fn(hash_table->items[index].key, key)) {
      return hash_table->items[index].value;
    }
    index++;
    if (index >= hash_table->capacity) {
      index = 0;
    }
  }

  return NULL;
}

bool HashTable_has(const HashTable *hash_table, const void *key) {
  ASSERT(hash_table != NULL);
  ASSERT(key != NULL);
  size_t index = HashTable_index_for_key(hash_table, key);
  while (hash_table->items[index].is_present) {
    if (hash_table->key_eq_fn(hash_table->items[index].key, key)) {
      return true;
    }
    index++;
    if (index >= hash_table->capacity) {
      index = 0;
    }
  }

  return false;
}

size_t HashTable_length(const HashTable *hash_table) {
  ASSERT(hash_table != NULL);
  return hash_table->length;
}

void HashTable_steal(HashTable *hash_table, const void *key) {
  ASSERT(hash_table != NULL);
  ASSERT(key != NULL);
  size_t index = HashTable_index_for_key(hash_table, key);
  while (hash_table->items[index].is_present) {
    if (hash_table->key_eq_fn(hash_table->items[index].key, key)) {
      hash_table->key_dctor_fn(hash_table->allocator,
                               hash_table->items[index].key);
      hash_table->items[index].key = NULL;
      hash_table->items[index].value = NULL;
      hash_table->items[index].is_present = false;
      break;
    }
    index++;
    if (index >= hash_table->capacity) {
      index = 0;
    }
  }
  hash_table->length--;
}
void HashTable_clear(HashTable *hash_table) {
  ASSERT(hash_table != NULL);

  for (size_t i = 0; i < hash_table->capacity; i++) {
    if (hash_table->items[i].is_present) {
      hash_table->key_dctor_fn(hash_table->allocator, hash_table->items[i].key);
      hash_table->value_dctor_fn(hash_table->allocator,
                                 hash_table->items[i].value);
      hash_table->items[i].is_present = false;
    }
  }

  hash_table->length = 0;
}

bool HashSet_init(Allocator *allocator, HashSet *hash_set,
                  const size_t initial_capacity, HashTableKeyHashFn hash_fn,
                  HashTableKeyEqFn eq_fn) {
  ASSERT(allocator != NULL);
  ASSERT(hash_set != NULL);
  return HashTable_init(allocator, hash_set, initial_capacity, hash_fn, eq_fn);
}
bool HashSet_init_with_dctor(Allocator *allocator, HashSet *hash_set,
                             const size_t initial_capacity,
                             HashTableKeyHashFn hash_fn, HashTableKeyEqFn eq_fn,
                             HashTableDctorFn dctor_fn) {
  ASSERT(allocator != NULL);
  ASSERT(hash_set != NULL);
  return HashTable_init_with_dctors(allocator, hash_set, initial_capacity,
                                    hash_fn, eq_fn, dctor_fn,
                                    HashTable_noop_dctor_fn);
}
void HashSet_deinit(HashSet *hash_set) {
  ASSERT(hash_set != NULL);
  HashTable_deinit(hash_set);
}
bool HashSet_insert(HashSet *hash_set, void *set_value) {
  ASSERT(hash_set != NULL);
  return HashTable_insert(hash_set, set_value, NULL);
}
bool HashSet_has(const HashSet *hash_set, const void *set_value) {
  ASSERT(hash_set != NULL);
  return HashTable_has(hash_set, set_value);
}
size_t HashSet_length(const HashSet *hash_set) {
  ASSERT(hash_set != NULL);
  return HashTable_length(hash_set);
}
void HashSet_clear(HashSet *hash_set) {
  ASSERT(hash_set != NULL);
  HashTable_clear(hash_set);
}

struct HashTableIt {
  HashTable *table;
  size_t current_index;
  bool iterating;
};

HashTableIt *HashTableIt_create(Allocator *allocator, HashTable *table) {
  ASSERT(allocator != NULL);
  ASSERT(table != NULL);
  HashTableIt *it = allocator_allocate(allocator, sizeof(HashTableIt));
  it->table = table;
  it->current_index = 0;
  it->iterating = false;
  return it;
}

void HashTableIt_destroy(Allocator *allocator, HashTableIt *it) {
  ASSERT(allocator != NULL);
  ASSERT(it != NULL);
  allocator_free(allocator, it);
}

bool HashTableIt_next(HashTableIt *it) {
  ASSERT(it != NULL);
  if (it->table->length < 1) {
    return false;
  }

  if (it->iterating) {
    it->current_index++;
  } else {
    it->iterating = true;
  }

  if (it->current_index >= it->table->capacity) {
    return false;
  }

  while (!it->table->items[it->current_index].is_present) {
    it->current_index++;
    if (it->current_index >= it->table->capacity) {
      return false;
    }
  }

  return true;
}
void *HashTableIt_key(HashTableIt *it) {
  ASSERT(it != NULL);
  if (!it->iterating || it->current_index >= it->table->capacity)
    return NULL;

  if (!it->table->items[it->current_index].is_present) {
    return NULL;
  }

  return it->table->items[it->current_index].key;
}
void *HashTableIt_value(HashTableIt *it) {
  ASSERT(it != NULL);
  if (!it->iterating || it->current_index >= it->table->capacity)
    return NULL;

  if (!it->table->items[it->current_index].is_present) {
    return NULL;
  }

  return it->table->items[it->current_index].value;
}

HashTableIt *HashTable_iter(Allocator *allocator, HashTable *hash_table) {
  ASSERT(hash_table != NULL);
  return HashTableIt_create(allocator, hash_table);
}

uint64_t hash_fnv_1a(const char *bytes, size_t nbytes) {
  ASSERT(bytes != NULL);
  static const uint64_t FNV_OFFSET_BASIS = 14695981039346656037u;
  static const uint64_t FNV_PRIME = 1099511628211u;
  uint64_t hash = FNV_OFFSET_BASIS;
  for (size_t i = 0; i < nbytes; i++) {
    hash = hash ^ bytes[i];
    hash = hash * FNV_PRIME;
  }

  return hash;
}

uint64_t hash_str_hash(const void *str) {
  ASSERT(str != NULL);
  return hash_fnv_1a(str, strlen(str));
}

bool hash_str_eq(const void *a, const void *b) {
  ASSERT(a != NULL);
  ASSERT(b != NULL);
  return strcmp(a, b) == 0;
}
void hash_str_dctor(Allocator *allocator, void *str) {
  ASSERT(allocator != NULL);
  ASSERT(str != NULL);
  allocator_free(allocator, str);
}
