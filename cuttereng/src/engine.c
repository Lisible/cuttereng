#include "engine.h"
#include "assert.h"
#include "asset.h"
#include "event.h"
#include "image.h"
#include "log.h"
#include "memory.h"
#include "renderer/renderer.h"
#include "src/ecs/ecs.h"
#include "src/graphics/camera.h"
#include "src/input.h"
#include "src/math/matrix.h"

void engine_init(Engine *engine, const Configuration *configuration,
                 EcsInitSystem ecs_init_system, SDL_Window *window) {
  ASSERT(engine != NULL);
  ASSERT(configuration != NULL);
  ASSERT(window != NULL);
  ecs_init(&system_allocator, &engine->ecs, ecs_init_system);
  InputState_init(&engine->input_state);
  engine->assets = assets_new(&system_allocator);
  assets_register_loader(engine->assets, Image, &image_loader,
                         &image_destructor);

  engine->renderer = renderer_new(&system_allocator, window, engine->assets,
                                  engine->current_time_secs);
  engine->application_title = configuration->application_title;
  engine->running = true;
}

void engine_deinit(Engine *engine) {
  ASSERT(engine != NULL);
  ecs_deinit(&engine->ecs);
  assets_destroy(engine->assets);
  renderer_destroy(engine->renderer);

  allocator_free(&system_allocator, (char *)engine->application_title);
}

void engine_handle_events(Engine *engine, Event *event) {
  ASSERT(engine != NULL);
  ASSERT(event != NULL);
  switch (event->type) {
  case EVT_QUIT:
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

void engine_update(Allocator *frame_allocator, Engine *engine) {
  ASSERT(engine != NULL);
  (void)frame_allocator;

  const SystemContext system_context = {.current_time_secs =
                                            engine->current_time_secs};

  ecs_run_systems(&engine->ecs, &system_context);
  ecs_process_command_queue(&engine->ecs);
}

void engine_emit_draw_commands(Allocator *allocator, Engine *engine);
void engine_render(Allocator *frame_allocator, Engine *engine) {
  ASSERT(engine != NULL);
  LOG_TRACE("render");

  engine_emit_draw_commands(frame_allocator, engine);
  renderer_render(frame_allocator, engine->renderer, engine->current_time_secs);
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
  mat4 inverse_transform_mat;
  transform_matrix(transform, inverse_transform_mat);
  mat4_transpose(inverse_transform_mat);
  mat4 view_projection_matrix = {0};
  mat4_mul(projection, inverse_transform_mat, view_projection_matrix);
  renderer_set_projection(engine->renderer, view_projection_matrix);

  EcsQuery *query_cubes = EcsQuery_new(
      allocator,
      &(const EcsQueryDescriptor){
          .components = {ecs_component_id(Transform), ecs_component_id(Cube)},
          .component_count = 2});
  EcsQueryIt query_it = ecs_query(&engine->ecs, query_cubes);
  while (ecs_query_it_next(&query_it)) {
    Transform *transform = ecs_query_it_get(&query_it, Transform, 0);
    renderer_draw_mesh(engine->renderer, transform, "water.json");
  }
  EcsQuery_destroy(query_cubes, allocator);
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
