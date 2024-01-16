#ifndef CUTTERENG_ECS_ECS_H
#define CUTTERENG_ECS_ECS_H

#include "../common.h"
#include "../hash.h"
#include "../vec.h"

typedef size_t EcsId;
typedef struct Ecs Ecs;

typedef struct ComponentStore ComponentStore;
DECL_HASH_TABLE(ComponentStore, HashTableComponentStore)

typedef struct {
  const char **components;
} EcsQuery;

typedef struct EcsQueryItState EcsQueryItState;
typedef struct {
  Allocator *allocator;
  EcsQueryItState *state;
} EcsQueryIt;

typedef void (*EcsSystemFn)(EcsQueryIt *);

typedef struct {
  EcsQuery query;
  EcsSystemFn fn;
} EcsSystemDescriptor;

typedef struct {
  EcsQuery query;
  EcsSystemFn fn;
} EcsSystem;

DECL_VEC(EcsSystem, EcsSystemVec)

struct Ecs {
  Allocator *allocator;
  HashTableComponentStore *component_stores;
  size_t entity_count;
  EcsSystemVec systems;
};

void ecs_init(Allocator *allocator, Ecs *ecs);
void ecs_deinit(Ecs *ecs);
void ecs_register_system(Ecs *ecs,
                         const EcsSystemDescriptor *system_descriptor);
void ecs_run_systems(Ecs *ecs);
EcsId ecs_create_entity(Ecs *ecs);
size_t ecs_get_entity_count(const Ecs *ecs);
void ecs_insert_component_(Ecs *ecs, EcsId entity_id, char *component_name,
                           size_t component_size, const void *data);
bool ecs_has_component_(const Ecs *ecs, EcsId entity_id,
                        const char *component_name);
void *ecs_get_component_(const Ecs *ecs, EcsId entity_id,
                         const char *component_name);
size_t ecs_count_matching(const Ecs *ecs, const EcsQuery *query);
EcsQueryIt ecs_query(const Ecs *ecs, const EcsQuery *query);
bool ecs_query_is_matching(const Ecs *ecs, const EcsQuery *query,
                           EcsId entity_id);
bool ecs_query_it_next(EcsQueryIt *it);
void *ecs_query_it_get_(const EcsQueryIt *it, size_t component);
EcsId ecs_query_it_entity_id(const EcsQueryIt *it);
void ecs_query_it_deinit(EcsQueryIt *it);

#define ecs_insert_component(ecs, entity_id, component_type, ...)              \
  ecs_insert_component_(ecs, entity_id, #component_type,                       \
                        sizeof(component_type), &(component_type)__VA_ARGS__)

#define ecs_component_id(component_type) #component_type

#define ecs_has_component(ecs, entity_id, component_type)                      \
  ecs_has_component_(ecs, entity_id, ecs_component_id(component_type))
#define ecs_get_component(ecs, entity_id, component_type)                      \
  (component_type *)ecs_get_component_(ecs, entity_id,                         \
                                       ecs_component_id(component_type))

#define ecs_query_it_get(it, component_type, index)                            \
  (component_type *)ecs_query_it_get_(it, index)

#endif // CUTTERENG_ECS_ECS_H
