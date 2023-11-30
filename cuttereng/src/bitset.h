#ifndef CUTTERENG_BITSET_H
#define CUTTERENG_BITSET_H

#include "common.h"
#define WORD_BITS (8 * sizeof(unsigned int))

static inline void bitset_set(unsigned int *bitset, size_t index) {
  bitset[index / WORD_BITS] |= (1 << (index % WORD_BITS));
}

static inline void bitset_unset(unsigned int *bitset, size_t index) {
  bitset[index / WORD_BITS] &= ~(1 << (index % WORD_BITS));
}
static inline bool bitset_is_set(unsigned int *bitset, size_t index) {
  return (bitset[index / WORD_BITS] &= (1 << (index % WORD_BITS))) != 0;
}

#endif // CUTTERENG_BITSET_H
