#include "graphics/camera.h"
#include "input.h"
#include "math/quaternion.h"
#include "math/vector.h"
#include <cuttereng.h>
#include <ecs/ecs.h>
#include <engine.h>
#include <graphics/shape.h>
#include <transform.h>

typedef struct Moving Moving;

void system_move_cubes(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;

  const SystemContext *system_context = it->ctx;
  float dt = system_context->delta_time_secs;
  while (ecs_query_it_next(it)) {
    Transform *transform = ecs_query_it_get(it, Transform, 0);
    transform->position.z += sin(system_context->current_time_secs) * dt;
  }
}

void system_print_cube_infos(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  while (ecs_query_it_next(it)) {
    EcsId entity_id = ecs_query_it_entity_id(it);
    Transform *transform = ecs_query_it_get(it, Transform, 0);
    LOG_DEBUG("Entity %d has position (%f, %f, %f)", entity_id,
              transform->position.x, transform->position.y,
              transform->position.z);
    LOG_DEBUG("Entity %d has cube", entity_id);
  }
}

void system_rotate_camera(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  const SystemContext *system_context = it->ctx;
  InputState *input_state = system_context->input_state;
  float mouse_motion_x = input_state->last_mouse_motion.x;
  float mouse_motion_y = input_state->last_mouse_motion.y;
  if (!input_state->did_mouse_move) {
    mouse_motion_x = 0.0;
    mouse_motion_y = 0.0;
  }

  while (ecs_query_it_next(it)) {
    Transform *transform = ecs_query_it_get(it, Transform, 0);
    Quaternion y_rotation = {1, {0}};
    quaternion_set_to_axis_angle(&y_rotation, &(const v3f){.y = 1.0},
                                 mouse_motion_x * 0.01);
    Quaternion rotation = {1, {0}};
    memcpy(&rotation, &transform->rotation, sizeof(Quaternion));

    Quaternion x_rotation = {1, {0}};
    quaternion_set_to_axis_angle(&x_rotation, &(const v3f){.x = 1.0},
                                 mouse_motion_y * 0.01);

    Quaternion result = {1.f, {0.f, 0.f, 0.f}};
    quaternion_mul(&result, &y_rotation);
    quaternion_mul(&result, &rotation);
    quaternion_mul(&result, &x_rotation);
    memcpy(&transform->rotation, &result, sizeof(Quaternion));
    // TODO fix this
    input_state->did_mouse_move = false;
  }
}

void system_move_camera(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  const SystemContext *system_context = it->ctx;
  InputState *input_state = system_context->input_state;
  float dt = system_context->delta_time_secs;
  float speed = 5.0;
  while (ecs_query_it_next(it)) {
    Transform *transform = ecs_query_it_get(it, Transform, 0);

    v3f forward = {0.0, 0.0, 1.0};
    quaternion_apply_to_vector(&transform->rotation, &forward);
    v3f_mul_scalar(&forward, speed * dt);
    v3f up = {0.0, 1.0, 0.0};
    quaternion_apply_to_vector(&transform->rotation, &up);
    v3f_mul_scalar(&up, speed * dt);
    v3f right = {1.0, 0.0, 0.0};
    quaternion_apply_to_vector(&transform->rotation, &right);
    v3f_mul_scalar(&right, speed * dt);

    if (InputState_is_key_down(input_state, Key_W)) {
      v3f_add(&transform->position, &forward);
    }
    if (InputState_is_key_down(input_state, Key_S)) {
      v3f_sub(&transform->position, &forward);
    }
    if (InputState_is_key_down(input_state, Key_D)) {
      v3f_add(&transform->position, &right);
    }
    if (InputState_is_key_down(input_state, Key_A)) {
      v3f_sub(&transform->position, &right);
    }
    if (InputState_is_key_down(input_state, Key_Q)) {
      v3f_add(&transform->position, &up);
    }
    if (InputState_is_key_down(input_state, Key_E)) {
      v3f_sub(&transform->position, &up);
    }
  }
}

void init_system(EcsCommandQueue *command_queue) {
  LOG_DEBUG("Initializing");
  ecs_command_queue_register_system(
      command_queue,
      &(const EcsSystemDescriptor){
          .query =
              (EcsQueryDescriptor){.components = {ecs_component_id(Transform),
                                                  ecs_component_id(Cube),
                                                  ecs_component_id(Moving)},
                                   .component_count = 3},
          .fn = system_move_cubes});
  ecs_command_queue_register_system(
      command_queue,
      &(const EcsSystemDescriptor){
          .query =
              (EcsQueryDescriptor){.components = {ecs_component_id(Transform),
                                                  ecs_component_id(Cube)},
                                   .component_count = 2},
          .fn = system_print_cube_infos});
  ecs_command_queue_register_system(
      command_queue,
      &(const EcsSystemDescriptor){
          .query =
              (EcsQueryDescriptor){.components = {ecs_component_id(Transform),
                                                  ecs_component_id(Camera)},
                                   .component_count = 2},
          .fn = system_rotate_camera});
  ecs_command_queue_register_system(
      command_queue,
      &(const EcsSystemDescriptor){
          .query =
              (EcsQueryDescriptor){.components = {ecs_component_id(Transform),
                                                  ecs_component_id(Camera)},
                                   .component_count = 2},
          .fn = system_move_camera});

  EcsId camera_entity = ecs_command_queue_create_entity(command_queue);
  Camera camera;
  camera_init_perspective(&camera, 45.f, 800.f / 600.f, 0.1f, 100.f);
  ecs_command_queue_insert_component_with_ptr(command_queue, camera_entity,
                                              Camera, &camera);
  Transform camera_transform = TRANSFORM_DEFAULT;
  ecs_command_queue_insert_component_with_ptr(command_queue, camera_entity,
                                              Transform, &camera_transform);

  EcsId first_entity = ecs_command_queue_create_entity(command_queue);
  Transform first_entity_transform = TRANSFORM_DEFAULT;
  first_entity_transform.position.z = 10.0;
  first_entity_transform.position.x = -3.0;
  first_entity_transform.position.y = 0.0;
  ecs_command_queue_insert_component_with_ptr(
      command_queue, first_entity, Transform, &first_entity_transform);
  ecs_command_queue_insert_tag_component(command_queue, first_entity, Cube);
  ecs_command_queue_insert_tag_component(command_queue, first_entity, Moving);

  EcsId second_entity = ecs_command_queue_create_entity(command_queue);
  Transform second_entity_transform = TRANSFORM_DEFAULT;
  second_entity_transform.position.z = 3.0;
  second_entity_transform.position.x = 1.0;
  second_entity_transform.position.y = 0.0;
  ecs_command_queue_insert_component_with_ptr(
      command_queue, second_entity, Transform, &second_entity_transform);
  ecs_command_queue_insert_tag_component(command_queue, second_entity, Cube);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  cutter_bootstrap(init_system);
  return 0;
}
