#include "engine.h"
#include "assert.h"
#include "asset.h"
#include "event.h"
#include "image.h"
#include "log.h"
#include "memory.h"
#include "renderer/material.h"
#include "renderer/renderer.h"
#include "src/ecs/ecs.h"
#include "src/graphics/camera.h"
#include "src/graphics/light.h"
#include "src/graphics/mesh_instance.h"
#include "src/input.h"
#include "src/math/matrix.h"

void engine_init(Engine *engine, const Configuration *configuration,
                 EcsSystemFn ecs_init_system, SDL_Window *window) {
  ASSERT(engine != NULL);
  ASSERT(configuration != NULL);
  ASSERT(window != NULL);
  InputState_init(&engine->input_state);
  engine->assets = assets_new(&system_allocator);
  assets_register_loader(engine->assets, Image, &image_loader,
                         &image_destructor);

  engine->renderer = renderer_new(&system_allocator, window, engine->assets);
  engine->application_title = configuration->application_title;
  engine->running = true;
  engine->capturing_mouse = false;
  ecs_init(&system_allocator, &engine->ecs, ecs_init_system,
           &(SystemContext){.input_state = &engine->input_state,
                            .assets = engine->assets,
                            .current_time_secs = engine->current_time_secs,
                            .delta_time_secs = 0});
}

void engine_deinit(Engine *engine) {
  ASSERT(engine != NULL);
  ecs_deinit(&engine->ecs);
  assets_destroy(engine->assets);
  renderer_destroy(engine->renderer);

  allocator_free(&system_allocator, (char *)engine->application_title);
}

void engine_reload_assets(Engine *engine) { assets_clear(engine->assets); }

void engine_handle_events(Engine *engine, Event *event) {
  ASSERT(engine != NULL);
  ASSERT(event != NULL);
  switch (event->type) {
  case EventType_KeyDown:
    if (event->key_event.key == Key_F1) {
      engine_reload_assets(engine);
      renderer_clear_caches(engine->renderer);
    } else {
      InputState_handle_event(&engine->input_state, event);
    }

    if (event->key_event.key == Key_F2) {
      engine->capturing_mouse = !engine->capturing_mouse;
    }

    break;
  case EventType_Quit:
    engine->running = false;
    break;
  default:
    InputState_handle_event(&engine->input_state, event);
    break;
  }
}
void engine_set_current_time(Engine *engine, float current_time_secs) {
  ASSERT(engine != NULL);
  engine->current_time_secs = current_time_secs;
}

void engine_update(Allocator *frame_allocator, Engine *engine, float dt) {
  ASSERT(engine != NULL);
  (void)frame_allocator;

  LOG_DEBUG("Delta time: %fs", dt);
  const SystemContext system_context = {.delta_time_secs = dt,
                                        .current_time_secs =
                                            engine->current_time_secs,
                                        .input_state = &engine->input_state,
                                        .assets = engine->assets};
  ecs_run_systems(&engine->ecs, &system_context);
  ecs_process_command_queue(&engine->ecs);
}

void engine_emit_draw_commands(Allocator *allocator, Engine *engine);
void engine_render(Allocator *frame_allocator, Engine *engine) {
  ASSERT(engine != NULL);
  LOG_TRACE("render");

  engine_emit_draw_commands(frame_allocator, engine);
  renderer_render(frame_allocator, engine->renderer, engine->assets,
                  engine->current_time_secs);
  InputState_on_frame_end(&engine->input_state);
}

bool engine_is_running(Engine *engine) {
  ASSERT(engine != NULL);
  return engine->running;
}

void engine_emit_draw_commands(Allocator *allocator, Engine *engine) {
  EcsQuery *query_camera = EcsQuery_new(
      allocator,
      &(const EcsQueryDescriptor){
          .components = {ecs_component_id(Camera), ecs_component_id(Transform)},
          .component_count = 2});
  EcsQueryIt camera_query_it = ecs_query(&engine->ecs, query_camera);
  ecs_query_it_next(&camera_query_it);
  Camera *camera = ecs_query_it_get(&camera_query_it, Camera, 0);
  Transform *transform = ecs_query_it_get(&camera_query_it, Transform, 1);
  mat4 projection = {0};
  memcpy(projection, camera->projection_matrix, 16 * sizeof(mat4_value_type));
  mat4 transform_mat;
  transform_matrix(transform, transform_mat);
  mat4 inverse_transform_mat;
  mat4_transpose(inverse_transform_mat);
  mat4_inverse(transform_mat, inverse_transform_mat);
  mat4 view_projection_matrix = {0};
  mat4_mul(projection, inverse_transform_mat, view_projection_matrix);
  LOG_DEBUG("view_position: %f,%f,%f", transform->position.x,
            transform->position.y, transform->position.z);
  renderer_set_view_position(engine->renderer, &transform->position);
  renderer_set_view_projection(engine->renderer, view_projection_matrix);
  ecs_query_it_deinit(&camera_query_it);
  EcsQuery_destroy(query_camera, allocator);

  EcsQuery *query_directional_light = EcsQuery_new(
      allocator, &(const EcsQueryDescriptor){
                     .component_count = 1,
                     .components = {ecs_component_id(DirectionalLight)}});
  EcsQueryIt query_directional_light_it =
      ecs_query(&engine->ecs, query_directional_light);
  while (ecs_query_it_next(&query_directional_light_it)) {
    DirectionalLight *directional_light =
        ecs_query_it_get(&query_directional_light_it, DirectionalLight, 0);
    renderer_add_light(engine->renderer,
                       &(const Light){.type = LightType_Directional,
                                      .directional_light = *directional_light});
  }
  ecs_query_it_deinit(&query_directional_light_it);
  EcsQuery_destroy(query_directional_light, allocator);

  // {
  //   EcsQuery *query_meshes = EcsQuery_new(
  //       allocator, &(const EcsQueryDescriptor){
  //                      .components = {ecs_component_id(Transform),
  //                                     ecs_component_id(MeshInstance),
  //                                     ecs_component_id(ShaderMaterial)},
  //                      .component_count = 3});
  //   EcsQueryIt query_it = ecs_query(&engine->ecs, query_meshes);
  //   while (ecs_query_it_next(&query_it)) {
  //     Transform *transform = ecs_query_it_get(&query_it, Transform, 0);
  //     MeshInstance *mesh_instance =
  //         ecs_query_it_get(&query_it, MeshInstance, 1);
  //     ShaderMaterial *shader_material =
  //         ecs_query_it_get(&query_it, ShaderMaterial, 2);
  //     renderer_draw_mesh_with_shader_material(engine->renderer, transform,
  //                                             mesh_instance->mesh_handle,
  //                                             shader_material->shader_handle);
  //   }
  //   ecs_query_it_deinit(&query_it);
  //   EcsQuery_destroy(query_meshes, allocator);
  // }

  {
    EcsQuery *query_meshes = EcsQuery_new(
        allocator,
        &(const EcsQueryDescriptor){.components =
                                        {
                                            ecs_component_id(Transform),
                                            ecs_component_id(MeshInstance),
                                        },
                                    .component_count = 2});
    EcsQueryIt query_it = ecs_query(&engine->ecs, query_meshes);
    while (ecs_query_it_next(&query_it)) {
      Transform *transform = ecs_query_it_get(&query_it, Transform, 0);
      MeshInstance *mesh_instance =
          ecs_query_it_get(&query_it, MeshInstance, 1);
      // Material*material=
      //     ecs_query_it_get(&query_it, Material, 2);
      renderer_draw_mesh(engine->renderer, transform,
                         mesh_instance->mesh_handle,
                         mesh_instance->material_handle);
    }
    ecs_query_it_deinit(&query_it);
    EcsQuery_destroy(query_meshes, allocator);
  }
}

bool configuration_from_json(Json *configuration_json,
                             Configuration *output_configuration) {
  ASSERT(configuration_json != NULL);
  ASSERT(output_configuration != NULL);

  if (configuration_json->type != JSON_OBJECT) {
    LOG_ERROR("project_configuration.json's root should be an object");
    return false;
  }
  JsonObject *configuration_json_object = configuration_json->object;
  Json *title_json = json_object_get(configuration_json_object, "title");
  if (!title_json) {
    LOG_ERROR("no property title found in project_configuration.json");
    return false;
  }

  if (title_json->type != JSON_STRING) {
    LOG_ERROR("title is not a string");
    return false;
  }

  output_configuration->application_title = title_json->string;
  json_object_steal(configuration_json_object, "title");
  json_destroy_without_cleanup(&system_allocator, title_json);

  Json *window_size_json =
      json_object_get(configuration_json->object, "window_size");
  if (!window_size_json) {
    LOG_ERROR("no window_size property found in project_configuration.json");
    return false;
  }

  if (!window_size_from_json(window_size_json,
                             &output_configuration->window_size)) {
    return false;
  }

  return true;
}

bool window_size_from_json(Json *json_value, WindowSize *window_size) {
  ASSERT(json_value != NULL);
  ASSERT(window_size != NULL);

  if (json_value->type != JSON_OBJECT) {
    LOG_ERROR("window_size property is not an object");
    return false;
  }

  Json *width = json_object_get(json_value->object, "width");
  if (!width) {
    LOG_ERROR("window_size.width not found");
    return false;
  }

  if (width->type != JSON_NUMBER) {
    LOG_ERROR("window_size.width should be a number");
    return false;
  }
  window_size->width = width->number;

  Json *height = json_object_get(json_value->object, "height");
  if (!height) {
    LOG_ERROR("window_size.height not found");
    return false;
  }
  if (height->type != JSON_NUMBER) {
    LOG_ERROR("window_size.height should be a number");
    return false;
  }
  window_size->height = height->number;
  return true;
}

void configuration_debug_print(Configuration *configuration) {
  ASSERT(configuration != NULL);
  LOG_DEBUG("Configuration");
  LOG_DEBUG("title: %s", configuration->application_title);
  window_size_debug_print(&configuration->window_size);
}
void window_size_debug_print(WindowSize *window_size) {
  ASSERT(window_size != NULL);
  LOG_DEBUG("window size: (%d, %d)", window_size->width, window_size->height);
}
bool engine_should_mouse_be_captured(Engine *engine) {
  ASSERT(engine != NULL);
  return engine->capturing_mouse;
}
