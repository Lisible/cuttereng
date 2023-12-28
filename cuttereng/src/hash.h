#ifndef CUTTERENG_HASH_H
#define CUTTERENG_HASH_H

#include "common.h"

typedef struct HashTable HashTable;
typedef struct {
  char *key;
  void *value;
} HashTableKV;

typedef void (*ItemDestructorFn)(void *);

/// Creates a new, empty `hash_table`
///
/// @param item_destructor The function used to destroy an item
///        if NULL is provided, `memory_free` will be used
/// @return The newly created `hash_table`
HashTable *hash_table_new(ItemDestructorFn item_destructor);

/// Destroys an `hash_table`
void hash_table_destroy(HashTable *table);

/// Sets a value into the hash_table
///
/// @param hash_table The hash table to set the value in
/// @param key The key for the value
/// @param value A non-null value
/// @return The key of the newly inserted value, NULL if an error occured
const char *hash_table_set(HashTable *table, char *key, void *value);

/// Returns the item for the given key
///
/// @return The item or NULL if it is not found
void *hash_table_get(const HashTable *table, const char *key);

/// Removes an item from the hash table
void hash_table_remove(HashTable *table, const char *key);

/// Removes an item without calling the item destructor
void hash_table_steal(HashTable *table, const char *key);

/// Clears the hash table
void hash_table_clear(HashTable *table);

/// @return true if an item is present for the given key
bool hash_table_has(const HashTable *table, const char *key);

/// @return the hash table length
size_t hash_table_length(const HashTable *table);

#define HashTableOf(T) HashTable_##T
#define DefineHashTableOf(T, item_destructor)                                  \
  typedef struct HashTableOf(T) { HashTable *internal_table; }                 \
  HashTable_##T;                                                               \
  void hash_table_##T##_item_destructor(void *item) { item_destructor(item); } \
                                                                               \
  void hash_table_##T##_init(HashTableOf(T) * table) {                         \
    table->internal_table = hash_table_new(hash_table_##T##_item_destructor);  \
  }                                                                            \
  void hash_table_##T##_clear(HashTableOf(T) * table) {                        \
    hash_table_clear(table->internal_table);                                   \
  }                                                                            \
  void hash_table_##T##_deinit(HashTableOf(T) * table) {                       \
    hash_table_destroy(table->internal_table);                                 \
  }                                                                            \
  const char *hash_table_##T##_set(HashTableOf(T) * table, char *key,          \
                                   T *value) {                                 \
    return hash_table_set(table->internal_table, key, value);                  \
  }                                                                            \
  T *hash_table_##T##_get(const HashTableOf(T) * table, const char *key) {     \
    return hash_table_get(table->internal_table, key);                         \
  }                                                                            \
  void hash_table_##T##_steal(HashTableOf(T) * table, const char *key) {       \
    hash_table_steal(table->internal_table, key);                              \
  }                                                                            \
  void hash_table_##T##_remove(HashTableOf(T) * table, const char *key) {      \
    hash_table_remove(table->internal_table, key);                             \
  }                                                                            \
  bool hash_table_##T##_has(const HashTableOf(T) * table, const char *key) {   \
    return hash_table_has(table->internal_table, key);                         \
  }                                                                            \
  size_t hash_table_##T##_length(const HashTableOf(T) * table) {               \
    return hash_table_length(table->internal_table);                           \
  }

uint64_t hash_fnv_1a(const char *bytes);

#endif // CUTTERENG_HASH_H
