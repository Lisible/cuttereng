#include "engine.h"
#include "assert.h"
#include "asset.h"
#include "event.h"
#include "image.h"
#include "log.h"
#include "memory.h"
#include "src/ecs/ecs.h"
#include "src/hash.h"
#include "src/input.h"
#include "src/math/matrix.h"
#include "src/transform.h"

void engine_init(Engine *engine, const Configuration *configuration,
                 EcsSystemFn ecs_init_system, SDL_Window *window) {
  ASSERT(engine != NULL);
  ASSERT(configuration != NULL);
  ASSERT(window != NULL);
  InputState_init(&engine->input_state);
  engine->assets = assets_new(&system_allocator);
  assets_register_asset_type(engine->assets, Image, &image_loader,
                             &image_deinitializer);

  engine->application_title = configuration->application_title;
  engine->running = true;
  engine->capturing_mouse = false;
  // FIXME The transform cache should grow as the number of entity grows
  engine->transform_cache =
      allocator_allocate_array(&system_allocator, 4096, sizeof(mat4));
  ecs_init(&system_allocator, &engine->ecs, ecs_init_system,
           &(SystemContext){.input_state = &engine->input_state,
                            .assets = engine->assets,
                            .current_time_secs = engine->current_time_secs,
                            .delta_time_secs = 0});
}

void engine_deinit(Engine *engine) {
  ASSERT(engine != NULL);
  ecs_deinit(&engine->ecs);
  allocator_free(&system_allocator, engine->transform_cache);
  assets_destroy(engine->assets);
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
DECL_VEC(EcsId, EcsIdVec)
DEF_VEC(EcsId, EcsIdVec, 512)

void engine_update_transform_cache(Engine *engine) {
  ASSERT(engine != NULL);
  size_t entity_count = ecs_get_entity_count(&engine->ecs);
  EcsIdVec queue;
  EcsIdVec_init(&system_allocator, &queue);

  EcsIdVec ord;
  EcsIdVec_init(&system_allocator, &ord);

  HashSet updated_ids = {0};
  HashSet_init_with_dctor(&system_allocator, &updated_ids, 128, ecs_id_hash_fn,
                          ecs_id_eq_fn, ecs_id_dctor_fn);

  for (size_t i = 0; i < entity_count; i++) {
    HashSet *targets = ecs_get_relationship_targets(&engine->ecs, i, ChildOf);
    if (!targets || (HashSet_length(targets) == 0)) {
      EcsIdVec_push_back(&queue, i);
    }
  }

  while (queue.length > 0) {
    EcsId n = EcsIdVec_pop_back(&queue);
    EcsIdVec_push_back(&ord, n);
    HashSet *children = ecs_get_relationship_sources(&engine->ecs, ChildOf, n);
    if (!children || HashSet_length(children) < 1)
      continue;

    HashTableIt *it = HashTable_iter(&system_allocator, children);
    while (HashTableIt_next(it)) {
      EcsIdVec_push_back(&queue, *(size_t *)HashTableIt_key(it));
    }
    HashTableIt_destroy(&system_allocator, it);
  }

  for (size_t i = 0; i < ord.length; i++) {
    EcsId entity_id = ord.data[i];
    LOG_DEBUG("Current entity: %zu", entity_id);
    Transform *transform =
        ecs_get_component(&engine->ecs, entity_id, Transform);
    float *parent_transform_mat = (float[])MAT4_IDENTITY;

    bool parent_dirty = false;
    HashSet *targets =
        ecs_get_relationship_targets(&engine->ecs, entity_id, ChildOf);
    if (targets) {
      HashTableIt *it = HashTable_iter(&system_allocator, targets);
      if (HashTableIt_next(it)) {
        EcsId *parent_id = HashTableIt_key(it);
        if (HashSet_has(&updated_ids, parent_id)) {
          parent_dirty = true;
        }
        parent_transform_mat = (float *)engine->transform_cache[*parent_id];
      }
      HashTableIt_destroy(&system_allocator, it);
    }

    if (!transform->dirty && !parent_dirty) {
      continue;
    }

    EcsId *owned_id = malloc(sizeof(EcsId));
    *owned_id = entity_id;
    HashSet_insert(&updated_ids, owned_id);

    mat4 entity_transform_mat = MAT4_IDENTITY;
    transform_matrix(transform, entity_transform_mat);
    mat4_mul(entity_transform_mat, parent_transform_mat,
             engine->transform_cache[entity_id]);

    transform->dirty = false;
  }

  HashSet_deinit(&updated_ids);
  EcsIdVec_deinit(&ord);
  EcsIdVec_deinit(&queue);
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
  engine_update_transform_cache(engine);
}

void engine_render(Allocator *frame_allocator, Engine *engine) {
  ASSERT(engine != NULL);
  (void)frame_allocator;
  InputState_on_frame_end(&engine->input_state);
}

bool engine_is_running(Engine *engine) {
  ASSERT(engine != NULL);
  return engine->running;
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
