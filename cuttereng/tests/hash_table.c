#include "test.h"
#include <hash.h>
#include <memory.h>

static void hash_table_create(void) {
  HashTable hash_table;
  T_ASSERT(HashTable_init(&system_allocator, &hash_table, 16, hash_str_hash,
                          hash_str_eq));
  T_ASSERT_EQ(hash_table.capacity, 16);
  T_ASSERT_EQ(hash_table.length, 0);
  HashTable_deinit(&hash_table);
}

static void hash_table_insert(void) {
  HashTable hash_table;
  T_ASSERT(HashTable_init(&system_allocator, &hash_table, 16, hash_str_hash,
                          hash_str_eq));
  T_ASSERT_EQ(hash_table.length, 0);

  char *first_key = "first_key";
  int first_value = 5;
  T_ASSERT(HashTable_insert(&hash_table, first_key, &first_value));
  T_ASSERT_EQ(hash_table.length, 1);

  char *second_key = "second_key";
  int second_value = 7;
  T_ASSERT(HashTable_insert(&hash_table, second_key, &second_value));
  char *third_key = "third_key";
  int third_value = 23;
  T_ASSERT(HashTable_insert(&hash_table, third_key, &third_value));
  T_ASSERT_EQ(hash_table.length, 3);
  HashTable_deinit(&hash_table);
}

static void hash_table_get(void) {
  HashTable hash_table;
  T_ASSERT(HashTable_init(&system_allocator, &hash_table, 16, hash_str_hash,
                          hash_str_eq));
  char *first_key = "first_key";
  int first_value = 5;
  T_ASSERT(HashTable_insert(&hash_table, first_key, &first_value));
  char *second_key = "second_key";
  int second_value = 7;
  T_ASSERT(HashTable_insert(&hash_table, second_key, &second_value));
  char *third_key = "third_key";
  int third_value = 23;
  T_ASSERT(HashTable_insert(&hash_table, third_key, &third_value));

  int *returned_value = HashTable_get(&hash_table, "second_key");
  T_ASSERT(returned_value != NULL);
  T_ASSERT_EQ(*returned_value, 7);
  HashTable_deinit(&hash_table);
}

static void hash_table_has(void) {
  HashTable hash_table;
  T_ASSERT(HashTable_init(&system_allocator, &hash_table, 16, hash_str_hash,
                          hash_str_eq));
  char *first_key = "first_key";
  int first_value = 5;
  T_ASSERT(HashTable_insert(&hash_table, first_key, &first_value));
  char *second_key = "second_key";
  int second_value = 7;
  T_ASSERT(HashTable_insert(&hash_table, second_key, &second_value));
  char *third_key = "third_key";
  int third_value = 23;
  T_ASSERT(HashTable_insert(&hash_table, third_key, &third_value));

  T_ASSERT(HashTable_has(&hash_table, "first_key"));
  T_ASSERT(HashTable_has(&hash_table, "second_key"));
  T_ASSERT(!HashTable_has(&hash_table, "thrid_key"));
  T_ASSERT(HashTable_has(&hash_table, "third_key"));
  HashTable_deinit(&hash_table);
}

static void hash_table_expand(void) {
  HashTable hash_table;
  T_ASSERT(HashTable_init(&system_allocator, &hash_table, 4, hash_str_hash,
                          hash_str_eq));
  char *first_key = "first_key";
  int first_value = 5;
  T_ASSERT(HashTable_insert(&hash_table, first_key, &first_value));
  char *second_key = "second_key";
  int second_value = 7;
  T_ASSERT(HashTable_insert(&hash_table, second_key, &second_value));
  char *third_key = "third_key";
  int third_value = 23;
  T_ASSERT(HashTable_insert(&hash_table, third_key, &third_value));
  char *fourth_key = "fourth_key";
  int fourth_value = 24;
  T_ASSERT(HashTable_insert(&hash_table, fourth_key, &fourth_value));
  char *fifth_key = "fifth_key";
  int fifth_value = 25;
  T_ASSERT(HashTable_insert(&hash_table, fifth_key, &fifth_value));

  T_ASSERT(HashTable_has(&hash_table, "first_key"));
  T_ASSERT(HashTable_has(&hash_table, "second_key"));
  T_ASSERT(!HashTable_has(&hash_table, "thrid_key"));
  T_ASSERT(HashTable_has(&hash_table, "third_key"));
  T_ASSERT(HashTable_has(&hash_table, "fourth_key"));
  T_ASSERT(HashTable_has(&hash_table, "fifth_key"));
  HashTable_deinit(&hash_table);
}

static void str_dctor_fn(Allocator *allocator, void *v) {
  allocator_free(allocator, v);
}

static void hash_table_heap_allocated_str(void) {
  HashTable hash_table;
  T_ASSERT(HashTable_init_with_dctors(&system_allocator, &hash_table, 16,
                                      hash_str_hash, hash_str_eq,
                                      HashTable_noop_dctor_fn, str_dctor_fn));

  char *first_value = calloc(8, sizeof(char));
  strcpy(first_value, "bonjour");

  T_ASSERT(HashTable_insert(&hash_table, "first_key", first_value));

  char *returned_value = HashTable_get(&hash_table, "first_key");
  T_ASSERT(strcmp(returned_value, "bonjour") == 0);
  HashTable_deinit(&hash_table);
}

static uint64_t int_hash_fn(const void *v) { return *(uint64_t *)v; }
static bool int_eq_fn(const void *a, const void *b) {
  return *(uint64_t *)a == *(uint64_t *)b;
}

static void hash_set_init(void) {
  HashSet set;
  HashSet_init(&system_allocator, &set, 16, int_hash_fn, int_eq_fn);
  T_ASSERT_EQ(HashSet_length(&set), 0u);
  HashSet_deinit(&set);
}

static void hash_set_insert(void) {
  HashSet set;
  HashSet_init(&system_allocator, &set, 16, int_hash_fn, int_eq_fn);
  T_ASSERT_EQ(HashSet_length(&set), 0u);

  uint64_t a = 15;
  HashSet_insert(&set, &a);
  T_ASSERT_EQ(HashSet_length(&set), 1u);
  uint64_t b = 26;
  HashSet_insert(&set, &b);
  uint64_t c = 28;
  HashSet_insert(&set, &c);
  T_ASSERT_EQ(HashSet_length(&set), 3u);
  HashSet_deinit(&set);
}

static void hash_set_has(void) {
  HashSet set;
  HashSet_init(&system_allocator, &set, 16, int_hash_fn, int_eq_fn);
  T_ASSERT_EQ(HashSet_length(&set), 0u);

  uint64_t a = 15;
  HashSet_insert(&set, &a);
  uint64_t b = 26;
  HashSet_insert(&set, &b);
  uint64_t c = 28;
  HashSet_insert(&set, &c);

  uint64_t d = 0;
  T_ASSERT(HashSet_has(&set, &a));
  T_ASSERT(HashSet_has(&set, &b));
  T_ASSERT(HashSet_has(&set, &c));
  T_ASSERT(!HashSet_has(&set, &d));
  T_ASSERT_EQ(HashSet_length(&set), 3u);
  HashSet_deinit(&set);
}

TEST_SUITE(TEST(hash_table_create), TEST(hash_table_insert),
           TEST(hash_table_get), TEST(hash_table_heap_allocated_str),
           TEST(hash_table_has), TEST(hash_table_expand), TEST(hash_set_init),
           TEST(hash_set_insert), TEST(hash_set_has))
