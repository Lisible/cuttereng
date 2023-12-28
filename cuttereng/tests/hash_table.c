#include "test.h"
#include <hash.h>
#include <memory.h>

DECL_HASH_TABLE(int, HashTableInt)
DEF_HASH_TABLE(int, HashTableInt, memory_free)

void t_hash_table_create() {
  HashTableInt *hash_table = HashTableInt_create(16);
  ASSERT_EQ(HashTableInt_length(hash_table), 0);
  HashTableInt_destroy(hash_table);
}

void t_hash_table_set() {
  HashTableInt *hash_table = HashTableInt_create(16);
  int *i = malloc(sizeof(int));
  *i = 0;
  HashTableInt_set(hash_table, "some_key", i);
  ASSERT_EQ(HashTableInt_length(hash_table), 1);
  HashTableInt_destroy(hash_table);
}

void t_hash_table_has() {
  HashTableInt *hash_table = HashTableInt_create(16);
  int *i = malloc(sizeof(int));
  *i = 532;
  HashTableInt_set(hash_table, "some_key", i);
  ASSERT(HashTableInt_has(hash_table, "some_key"));
  HashTableInt_destroy(hash_table);
}

void t_hash_table_get() {
  HashTableInt *hash_table = HashTableInt_create(16);
  int *i = malloc(sizeof(int));
  *i = 532;
  HashTableInt_set(hash_table, "some_key", i);
  ASSERT_EQ(*HashTableInt_get(hash_table, "some_key"), 532);
  HashTableInt_destroy(hash_table);
}

TEST_SUITE(TEST(t_hash_table_create), TEST(t_hash_table_set),
           TEST(t_hash_table_has), TEST(t_hash_table_get));
