#include "graphics/camera.h"
#include "graphics/light.h"
#include "graphics/mesh_instance.h"
#include "input.h"
#include "math/quaternion.h"
#include "math/vector.h"
#include "renderer/material.h"
#include "renderer/mesh.h"
#include <cuttereng.h>
#include <ecs/ecs.h>
#include <engine.h>
#include <graphics/shape.h>
#include <transform.h>

typedef struct Moving Moving;
typedef struct Rotating Rotating;

float light_angle = M_PI_4;

void system_move_cubes(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;

  const SystemContext *system_context = it->ctx;
  while (ecs_query_it_next(it)) {
    Transform *transform = ecs_query_it_get(it, Transform, 0);
    transform->position.y =
        2.0 + sin(system_context->current_time_secs * 5.0 +
                  transform->position.x + transform->position.z);
  }
}
void system_move_light(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;

  const SystemContext *system_context = it->ctx;
  if (InputState_is_key_down(system_context->input_state, Key_P)) {
    LOG_DEBUG("Pressing P");
    light_angle += 0.05;
  } else if (InputState_is_key_down(system_context->input_state, Key_O)) {
    light_angle -= 0.05;
  }

  LOG_DEBUG("i: %f", light_angle);

  while (ecs_query_it_next(it)) {
    DirectionalLight *light = ecs_query_it_get(it, DirectionalLight, 0);
    light->position.x = 15.0 * sin(light_angle);
    light->position.z = 15.0 * cos(light_angle);
    LOG_DEBUG("Updated light position: %f %f %f", light->position.x,
              light->position.y, light->position.z);
  }
}
void system_rotate_cubes(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  while (ecs_query_it_next(it)) {
    Transform *transform = ecs_query_it_get(it, Transform, 0);
    Quaternion rot;
    quaternion_set_to_axis_angle(&rot, &(const v3f){.y = 1.0}, 0.03);
    quaternion_mul(&transform->rotation, &rot);
  }
}

void system_rotate_camera_mouse(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  const SystemContext *system_context = it->ctx;
  InputState *input_state = system_context->input_state;
  float motion_x = input_state->last_mouse_motion.x;
  float motion_y = input_state->last_mouse_motion.y;

  while (ecs_query_it_next(it)) {
    Transform *transform = ecs_query_it_get(it, Transform, 0);
    Quaternion y_rotation = {1, {0}};
    quaternion_set_to_axis_angle(&y_rotation, &(const v3f){.y = 1.0},
                                 motion_x * 0.02);
    Quaternion rotation = {1, {0}};
    memcpy(&rotation, &transform->rotation, sizeof(Quaternion));

    Quaternion x_rotation = {1, {0}};
    quaternion_set_to_axis_angle(&x_rotation, &(const v3f){.x = 1.0},
                                 motion_y * 0.02);

    Quaternion result = {1.f, {0.f, 0.f, 0.f}};
    quaternion_mul(&result, &y_rotation);
    quaternion_mul(&result, &rotation);
    quaternion_mul(&result, &x_rotation);
    memcpy(&transform->rotation, &result, sizeof(Quaternion));
  }
}

void system_rotate_camera_controller(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  const SystemContext *system_context = it->ctx;
  InputState *input_state = system_context->input_state;
  JoystickState *right_joystick = &input_state->controller.right_joystick_state;
  float motion_x = right_joystick->x;
  float motion_y = right_joystick->y;

  while (ecs_query_it_next(it)) {
    Transform *transform = ecs_query_it_get(it, Transform, 0);
    Quaternion y_rotation = {1, {0}};
    quaternion_set_to_axis_angle(&y_rotation, &(const v3f){.y = 1.0},
                                 motion_x * 0.02);
    Quaternion rotation = {1, {0}};
    memcpy(&rotation, &transform->rotation, sizeof(Quaternion));

    Quaternion x_rotation = {1, {0}};
    quaternion_set_to_axis_angle(&x_rotation, &(const v3f){.x = 1.0},
                                 motion_y * 0.02);

    Quaternion result = {1.f, {0.f, 0.f, 0.f}};
    quaternion_mul(&result, &y_rotation);
    quaternion_mul(&result, &rotation);
    quaternion_mul(&result, &x_rotation);
    memcpy(&transform->rotation, &result, sizeof(Quaternion));
  }
}

void system_move_camera_keyboard(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  const SystemContext *system_context = it->ctx;
  InputState *input_state = system_context->input_state;
  float dt = system_context->delta_time_secs;
  float speed = 5.0;
  while (ecs_query_it_next(it)) {
    Transform *transform = ecs_query_it_get(it, Transform, 0);

    v3f forward = {0.0, 0.0, 1.0};
    quaternion_apply_to_vector(&transform->rotation, &forward);
    v3f_mul_scalar(&forward, dt * speed);
    v3f up = {0.0, 1.0, 0.0};
    quaternion_apply_to_vector(&transform->rotation, &up);
    v3f_mul_scalar(&up, dt * speed);
    v3f right = {1.0, 0.0, 0.0};
    quaternion_apply_to_vector(&transform->rotation, &right);
    v3f_mul_scalar(&right, dt * speed);

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

void system_move_camera_controller(EcsCommandQueue *queue, EcsQueryIt *it) {
  (void)queue;
  const SystemContext *system_context = it->ctx;
  InputState *input_state = system_context->input_state;
  JoystickState *left_joystick = &input_state->controller.left_joystick_state;
  v3f direction_vector = {.x = left_joystick->x, .z = -left_joystick->y};
  float dt = system_context->delta_time_secs;
  float base_speed = 5.0;
  if (InputState_is_controller_button_down(input_state,
                                           ControllerButton_LShoulder)) {
    base_speed *= 3.0;
  }
  float vertical_speed = base_speed;
  float speed = base_speed * v3f_length(&direction_vector);

  while (ecs_query_it_next(it)) {
    Transform *transform = ecs_query_it_get(it, Transform, 0);

    quaternion_apply_to_vector(&transform->rotation, &direction_vector);
    v3f_mul_scalar(&direction_vector, dt * speed);
    if (fabs(left_joystick->x) > JOYSTICK_DEADZONE ||
        fabs(left_joystick->y) > JOYSTICK_DEADZONE) {
      v3f_add(&transform->position, &direction_vector);
    }

    v3f up = {0.0, 1.0, 0.0};
    quaternion_apply_to_vector(&transform->rotation, &up);
    v3f_mul_scalar(&up, dt * vertical_speed);

    if (InputState_is_controller_button_down(input_state, ControllerButton_A)) {
      v3f_add(&transform->position, &up);
    }
    if (InputState_is_controller_button_down(input_state, ControllerButton_B)) {
      v3f_sub(&transform->position, &up);
    }
  }
}

void init_system(EcsCommandQueue *command_queue, EcsQueryIt *it) {
  LOG_DEBUG("Initializing");
  SystemContext *ctx = it->ctx;
  ecs_command_queue_register_system(
      command_queue,
      &(const EcsSystemDescriptor){
          .query =
              (EcsQueryDescriptor){
                  .components = {ecs_component_id(DirectionalLight),
                                 ecs_component_id(Moving)},
                  .component_count = 2},
          .fn = system_move_light});
  ecs_command_queue_register_system(
      command_queue,
      &(const EcsSystemDescriptor){
          .query =
              (EcsQueryDescriptor){.components = {ecs_component_id(Transform),
                                                  ecs_component_id(Camera)},
                                   .component_count = 2},
          .fn = system_rotate_camera_controller});
  ecs_command_queue_register_system(
      command_queue,
      &(const EcsSystemDescriptor){
          .query =
              (EcsQueryDescriptor){.components = {ecs_component_id(Transform),
                                                  ecs_component_id(Camera)},
                                   .component_count = 2},
          .fn = system_rotate_camera_mouse});
  ecs_command_queue_register_system(
      command_queue,
      &(const EcsSystemDescriptor){
          .query =
              (EcsQueryDescriptor){.components = {ecs_component_id(Transform),
                                                  ecs_component_id(Camera)},
                                   .component_count = 2},
          .fn = system_move_camera_controller});
  ecs_command_queue_register_system(
      command_queue,
      &(const EcsSystemDescriptor){
          .query =
              (EcsQueryDescriptor){.components = {ecs_component_id(Transform),
                                                  ecs_component_id(Camera)},
                                   .component_count = 2},
          .fn = system_move_camera_keyboard});

  EcsId camera_entity = ecs_command_queue_create_entity(command_queue);
  Camera camera;
  camera_init_perspective(&camera, 45.f, 800.f / 600.f, 0.1f, 100.f);
  ecs_command_queue_insert_component_with_ptr(command_queue, camera_entity,
                                              Camera, &camera);
  Transform camera_transform = TRANSFORM_DEFAULT;
  camera_transform.position.x = -12.0;
  camera_transform.position.y = 7.0;
  camera_transform.position.z = -13.0;
  quaternion_set_to_axis_angle(&camera_transform.rotation,
                               &(const v3f){.y = 0.9, .x = 0.3}, M_PI_4);
  ecs_command_queue_insert_component_with_ptr(command_queue, camera_entity,
                                              Transform, &camera_transform);

  EcsId directional_light = ecs_command_queue_create_entity(command_queue);
  ecs_command_queue_insert_component(command_queue, directional_light,
                                     DirectionalLight,
                                     {.position = {0.0, 20.0, -10.0}});
  Transform directional_light_transform = TRANSFORM_DEFAULT;
  directional_light_transform.position.y = 1.0;
  ecs_command_queue_insert_component_with_ptr(command_queue, directional_light,
                                              Transform,
                                              &directional_light_transform);
  ecs_command_queue_insert_tag_component(command_queue, directional_light,
                                         Moving);

  EcsId ground = ecs_command_queue_create_entity(command_queue);
  Transform ground_transform = TRANSFORM_DEFAULT;
  ground_transform.position.x = 0.0;
  ground_transform.position.z = 0.0;
  ground_transform.position.y = 0.0;
  ground_transform.scale = (v3f){.x = 15.0, .y = 0.1, .z = 15.0};

  Mesh cube_mesh;
  cube_mesh_init(&cube_mesh);
  AssetHandle cube_mesh_handle =
      assets_store(ctx->assets, Mesh, "_cube", &cube_mesh);
  ecs_command_queue_insert_component(command_queue, ground, MeshInstance,
                                     {.mesh_handle = cube_mesh_handle});
  ecs_command_queue_insert_component_with_ptr(command_queue, ground, Transform,
                                              &ground_transform);

  AssetHandle water_material_handle;
  assets_load(ctx->assets, Material, "materials/water.json",
              &water_material_handle);
  ecs_command_queue_insert_component(
      command_queue, ground, Material,
      {.base_color = water_material_handle, .normal = water_material_handle});

  EcsId fox = ecs_command_queue_import_glb(command_queue, ctx->assets,
                                           "models/fox.glb");

  // EcsId fox = ecs_command_queue_create_entity(command_queue);
  // Transform fox_transform = TRANSFORM_DEFAULT;

  // AssetHandle fox_mesh_handle;
  // assets_load(ctx->assets, Mesh, "_cube", &cube_mesh);
  // ecs_command_queue_insert_component(command_queue, fox, MeshInstance,
  //                                    {.mesh_handle = cube_mesh_handle});
  // ecs_command_queue_insert_component_with_ptr(command_queue, fox, Transform,
  //                                             &fox_transform);
  // ecs_command_queue_insert_component(command_queue, fox, Material, {0});
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  cutter_bootstrap(init_system);
  return 0;
}
