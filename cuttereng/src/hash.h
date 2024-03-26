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
void *HashTable_get(const HashTable *hash_table, const void *key);
bool HashTable_has(const HashTable *hash_table, const void *key);
size_t HashTable_length(const HashTable *hash_table);
void HashTable_steal(HashTable *hash_table, const void *key);
void HashTable_clear(HashTable *hash_table);

void HashTable_noop_dctor_fn(Allocator *, void *v);

uint64_t hash_str_hash(const void *str);
bool hash_str_eq(const void *a, const void *b);
void hash_str_dctor(Allocator *allocator, void *str);

#endif // CUTTERENG_HASH_H
