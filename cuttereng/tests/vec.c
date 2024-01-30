#include "test.h"
#include <log.h>
#include <memory.h>
#include <vec.h>

VEC_IMPL(int, intvec, 128)

void t_vec_init(void) {
  intvec vec = {0};
  intvec_init(&system_allocator, &vec);
  T_ASSERT(intvec_length(&vec) == 0);
  T_ASSERT(intvec_capacity(&vec) == 128);
  intvec_deinit(&vec);
}

void t_vec_push_back(void) {
  intvec vec = {0};
  intvec_init(&system_allocator, &vec);
  T_ASSERT(intvec_length(&vec) == 0);
  intvec_push_back(&vec, 10);
  T_ASSERT(intvec_length(&vec) == 1);
  intvec_push_back(&vec, 13);
  intvec_push_back(&vec, 15);
  T_ASSERT(intvec_length(&vec) == 3);
  T_ASSERT(vec.data[0] == 10);
  T_ASSERT(vec.data[1] == 13);
  T_ASSERT(vec.data[2] == 15);
  intvec_deinit(&vec);
}
void t_vec_append(void) {
  intvec vec = {0};
  intvec_init(&system_allocator, &vec);
  T_ASSERT(intvec_length(&vec) == 0);
  intvec_push_back(&vec, 10);
  intvec_append(&vec, (int[]){11, 12, 13}, 3);
  T_ASSERT(intvec_length(&vec) == 4);
  T_ASSERT(vec.data[0] == 10);
  T_ASSERT(vec.data[1] == 11);
  T_ASSERT(vec.data[2] == 12);
  T_ASSERT(vec.data[3] == 13);
  intvec_deinit(&vec);
}

void t_vec_u8_append_1(void) {
  u8vec vec;
  u8vec_init(&system_allocator, &vec);
  u8 v = 1;
  for (size_t i = 0; i < 1000; i++)
    u8vec_append(&vec, &v, 1);
  u8vec_deinit(&vec);
}

TEST_SUITE(TEST(t_vec_init), TEST(t_vec_push_back), TEST(t_vec_append),
           TEST(t_vec_u8_append_1))
