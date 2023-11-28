#include "test.h"
#include <hash.h>

void t_hash_table_new() {
  hash_table *hash_table = hash_table_new(NULL);
  ASSERT_EQ(hash_table_length(hash_table), 0);
  hash_table_destroy(hash_table);
}

void t_hash_table_set() {
  hash_table *hash_table = hash_table_new(NULL);
  int *i = malloc(sizeof(int));
  *i = 0;
  hash_table_set(hash_table, "some_key", i);
  ASSERT_EQ(hash_table_length(hash_table), 1);
  hash_table_destroy(hash_table);
}

void t_hash_table_has() {
  hash_table *hash_table = hash_table_new(NULL);
  int *i = malloc(sizeof(int));
  *i = 532;
  hash_table_set(hash_table, "some_key", i);
  ASSERT(hash_table_has(hash_table, "some_key"));
  hash_table_destroy(hash_table);
}

void t_hash_table_get() {
  hash_table *hash_table = hash_table_new(NULL);
  int *i = malloc(sizeof(int));
  *i = 532;
  hash_table_set(hash_table, "some_key", i);
  ASSERT_EQ(*(int *)hash_table_get(hash_table, "some_key"), 532);
  hash_table_destroy(hash_table);
}

TEST_SUITE(TEST(t_hash_table_new), TEST(t_hash_table_set),
           TEST(t_hash_table_has), TEST(t_hash_table_get));
