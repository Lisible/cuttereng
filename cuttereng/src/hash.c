#include "hash.h"
#include "assert.h"
#include "memory.h"
#include <stddef.h>
#include <string.h>

void HashTable_noop_destructor(Allocator *allocator, void *value) {
  (void)allocator;
  (void)value;
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
