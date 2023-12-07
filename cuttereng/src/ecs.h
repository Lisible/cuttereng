#ifndef CUTTERENG_ECS_H
#define CUTTERENG_ECS_H

#include "common.h"
#include "hash.h"

typedef size_t EcsId;
typedef struct Ecs Ecs;
struct Ecs {
  HashTable *component_stores;
  size_t entity_count;
};

typedef struct {
  char **components;
} EcsQuery;

typedef struct EcsQueryItState EcsQueryItState;
typedef struct {
  EcsQueryItState *state;
} EcsQueryIt;

void ecs_init(Ecs *ecs);
void ecs_deinit(Ecs *ecs);
EcsId ecs_create_entity(Ecs *ecs);
size_t ecs_get_entity_count(const Ecs *ecs);
void ecs_insert_component(Ecs *ecs, EcsId entity_id, const char *component_name,
                          size_t component_size, const void *data);
bool ecs_has_component(const Ecs *ecs, EcsId entity_id,
                       const char *component_name);
void *ecs_get_component(const Ecs *ecs, EcsId entity_id,
                        const char *component_name);
size_t ecs_count_matching(const Ecs *ecs, const EcsQuery *query);
EcsQueryIt ecs_query(const Ecs *ecs, const EcsQuery *query);
bool ecs_query_is_matching(const Ecs *ecs, const EcsQuery *query,
                           EcsId entity_id);
bool ecs_query_it_next(EcsQueryIt *it);
void *ecs_query_it_get(const EcsQueryIt *it, size_t component);
EcsId ecs_query_it_entity_id(const EcsQueryIt *it);
void ecs_query_it_deinit(EcsQueryIt *it);

#define ecs_insert(ecs, entity_id, component_type, ...)                        \
  ecs_insert_component(ecs, entity_id, #component_type,                        \
                       sizeof(component_type), &(component_type)__VA_ARGS__)

#define ecs_component_id(component_type) #component_type

#define ecs_has(ecs, entity_id, component_type)                                \
  ecs_has_component(ecs, entity_id, ecs_component_id(component_type))
#define ecs_get(ecs, entity_id, component_type)                                \
  (component_type *)ecs_get_component(ecs, entity_id,                          \
                                      ecs_component_id(component_type))

#endif
