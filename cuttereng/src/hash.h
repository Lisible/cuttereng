#ifndef CUTTERENG_HASH_H
#define CUTTERENG_HASH_H

#include "common.h"

typedef struct hash_table hash_table;
typedef struct hash_table_kv hash_table_kv;
struct hash_table_kv {
  const char *key;
  void *value;
};
typedef void (*item_destructor_fn)(void *);

/// Creates a new, empty `hash_table`
///
/// @param item_destructor The function used to destroy an item
///        if NULL is provided, `memory_free` will be used
/// @return The newly created `hash_table`
hash_table *hash_table_new(item_destructor_fn item_destructor);

/// Destroys an `hash_table`
void hash_table_destroy(hash_table *table);

/// Sets a value into the hash_table
///
/// @param hash_table The hash table to set the value in
/// @param key The key for the value
/// @param value A non-null value
/// @return The key of the newly inserted value, NULL if an error occured
const char *hash_table_set(hash_table *table, const char *key, void *value);

/// Returns the item for the given key
///
/// @return The item or NULL if it is not found
void *hash_table_get(hash_table *table, const char *key);

/// Removes an item without calling the item destructor
void hash_table_steal(hash_table *table, const char *key);

/// @return true if an item is present for the given key
bool hash_table_has(hash_table *table, const char *key);

/// @return the hash table length
size_t hash_table_length(hash_table *table);

uint64_t hash_fnv_1a(const char *bytes);

#endif // CUTTERENG_HASH_H
