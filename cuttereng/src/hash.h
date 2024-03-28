#ifndef CUTTERENG_HASH_H
#define CUTTERENG_HASH_H

#include "memory.h"
#include <stdbool.h>

typedef struct {
  void *key;
  void *value;
  bool is_present;
} HashTableKV;

typedef uint64_t (*HashTableKeyHashFn)(const void *);
typedef void (*HashTableDctorFn)(Allocator *, void *);
typedef bool (*HashTableKeyEqFn)(const void *, const void *);
typedef struct {
  Allocator *allocator;
  HashTableKV *items;
  HashTableKeyHashFn key_hash_fn;
  HashTableKeyEqFn key_eq_fn;
  HashTableDctorFn key_dctor_fn;
  HashTableDctorFn value_dctor_fn;
  size_t capacity;
  size_t length;
} HashTable;

typedef struct HashTableIt HashTableIt;
void HashTableIt_destroy(Allocator *allocator, HashTableIt *it);
bool HashTableIt_next(HashTableIt *it);
void *HashTableIt_key(HashTableIt *it);
void *HashTableIt_value(HashTableIt *it);

bool HashTable_init(Allocator *allocator, HashTable *hash_table,
                    const size_t initial_capacity,
                    HashTableKeyHashFn key_hash_fn, HashTableKeyEqFn key_eq_fn);
bool HashTable_init_with_dctors(Allocator *allocator, HashTable *hash_table,
                                const size_t initial_capacity,
                                HashTableKeyHashFn key_hash_fn,
                                HashTableKeyEqFn key_eq_fn,
                                HashTableDctorFn key_dctor_fn,
                                HashTableDctorFn value_dctor_fn);
void HashTable_deinit(HashTable *hash_table);
bool HashTable_insert(HashTable *hash_table, void *key, void *value);
HashTableIt *HashTable_iter(Allocator *allocator, HashTable *hash_table);
void *HashTable_get(const HashTable *hash_table, const void *key);
bool HashTable_has(const HashTable *hash_table, const void *key);
size_t HashTable_length(const HashTable *hash_table);
void HashTable_steal(HashTable *hash_table, const void *key);
void HashTable_clear(HashTable *hash_table);

typedef HashTable HashSet;

bool HashSet_init(Allocator *allocator, HashSet *hash_set,
                  const size_t initial_capacity, HashTableKeyHashFn hash_fn,
                  HashTableKeyEqFn eq_fn);
bool HashSet_init_with_dctor(Allocator *allocator, HashSet *hash_set,
                             const size_t initial_capacity,
                             HashTableKeyHashFn hash_fn, HashTableKeyEqFn eq_fn,
                             HashTableDctorFn dctor_fn);
void HashSet_deinit(HashSet *hash_set);
bool HashSet_insert(HashSet *hash_set, void *set_value);
bool HashSet_has(const HashSet *hash_set, const void *set_value);
size_t HashSet_length(const HashSet *hash_set);
void HashSet_clear(HashSet *hash_set);

void HashTable_noop_dctor_fn(Allocator *, void *v);

uint64_t hash_str_hash(const void *str);
bool hash_str_eq(const void *a, const void *b);
void hash_str_dctor(Allocator *allocator, void *str);

#endif // CUTTERENG_HASH_H
