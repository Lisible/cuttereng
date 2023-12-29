#include "test.h"
#include <log.h>
#include <memory.h>
#include <vec.h>

VEC_IMPL(int, intvec, 128);

void t_vec_init() {
  intvec vec = {0};
  intvec_init(&system_allocator, &vec);
  ASSERT(intvec_length(&vec) == 0);
  ASSERT(intvec_capacity(&vec) == 128);
  intvec_deinit(&vec);
}

void t_vec_push_back() {
  intvec vec = {0};
  intvec_init(&system_allocator, &vec);
  ASSERT(intvec_length(&vec) == 0);
  intvec_push_back(&vec, 10);
  ASSERT(intvec_length(&vec) == 1);
  intvec_push_back(&vec, 13);
  intvec_push_back(&vec, 15);
  ASSERT(intvec_length(&vec) == 3);
  ASSERT(vec.data[0] == 10);
  ASSERT(vec.data[1] == 13);
  ASSERT(vec.data[2] == 15);
  intvec_deinit(&vec);
}
void t_vec_append() {
  intvec vec = {0};
  intvec_init(&system_allocator, &vec);
  ASSERT(intvec_length(&vec) == 0);
  intvec_push_back(&vec, 10);
  intvec_append(&vec, (int[]){11, 12, 13}, 3);
  ASSERT(intvec_length(&vec) == 4);
  ASSERT(vec.data[0] == 10);
  ASSERT(vec.data[1] == 11);
  ASSERT(vec.data[2] == 12);
  ASSERT(vec.data[3] == 13);
  intvec_deinit(&vec);
}

TEST_SUITE(TEST(t_vec_init), TEST(t_vec_push_back), TEST(t_vec_append))
