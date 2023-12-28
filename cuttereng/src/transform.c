#include "transform.h"
#include "assert.h"
#include "math/matrix.h"
#include "math/quaternion.h"

void transform_matrix(const Transform *transform, mat4 transform_matrix) {
  ASSERT(transform != NULL);
  mat4 translation_matrix = {0};
  mat4_set_to_translation(translation_matrix, &transform->position);
  mat4 rotation_matrix = {0};
  quaternion_rotation_matrix(&transform->rotation, rotation_matrix);
  mat4 scale_matrix = {0};
  mat4_set_to_scale(scale_matrix, &transform->scale);

  mat4 translation_rotation = {0};
  mat4_mul(translation_matrix, rotation_matrix, translation_rotation);
  mat4_mul(translation_rotation, scale_matrix, transform_matrix);
}
