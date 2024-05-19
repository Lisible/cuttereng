#ifndef CUTTERENG_ECS_ECS_H
#define CUTTERENG_ECS_ECS_H

#include "../asset.h"
#include <lisiblestd/hash.h>
#include <lisiblestd/vec.h>

typedef size_t EcsId;
typedef struct Ecs Ecs;

typedef struct ComponentStore ComponentStore;

#define ECS_QUERY_MAX_COMPONENT_COUNT 16

typedef struct {
  char *components[ECS_QUERY_MAX_COMPONENT_COUNT];
  size_t component_count;
} EcsQueryDescriptor;

typedef struct EcsQuery EcsQuery;
EcsQuery *EcsQuery_new(Allocator *allocator,
                       const EcsQueryDescriptor *query_descriptor);
void EcsQuery_destroy(EcsQuery *query, Allocator *allocator);
size_t EcsQuery_component_count(const EcsQuery *query);
char *EcsQuery_component(const EcsQuery *query, size_t component_index);

typedef struct EcsQueryItState EcsQueryItState;
typedef struct {
  Allocator *allocator;
  EcsQueryItState *state;
  const void *ctx;
} EcsQueryIt;

typedef struct EcsCommandQueue EcsCommandQueue;
typedef void (*EcsSystemFn)(EcsCommandQueue *, EcsQueryIt *);
typedef struct {
  EcsQueryDescriptor query;
  EcsSystemFn fn;
} EcsSystemDescriptor;

typedef enum {
  EcsCommandType_RegisterSystem,
  EcsCommandType_CreateEntity,
  EcsCommandType_InsertComponent,
  EcsCommandType_InsertRelationship,
} EcsCommandType;

typedef struct {
  EcsQuery *query;
  EcsSystemFn fn;
} EcsSystem;

typedef struct {
  EcsSystem system;
} EcsRegisterSystemCommand;

typedef struct {
  EcsId entity;
  char *component_name;
  size_t component_size;
  void *component_data;
} EcsInsertComponentCommand;

typedef struct {
  EcsId source;
  EcsId target;
  char *relationship_name;
} EcsInsertRelationshipCommand;

typedef struct {
  EcsCommandType type;
  union {
    EcsRegisterSystemCommand register_system;
    EcsInsertComponentCommand insert_component;
    EcsInsertRelationshipCommand insert_relationship;
  };
} EcsCommand;

void ecs_command_deinit(Allocator *allocator, EcsCommand *command);
void ecs_command_execute(Ecs *ecs, EcsCommand *command);

DECL_VEC(EcsCommand, EcsCommandVec)
struct EcsCommandQueue {
  Ecs *ecs;
  EcsCommandVec commands;
  Allocator *allocator;
};

void ecs_command_queue_init(Allocator *allocator, EcsCommandQueue *queue,
                            Ecs *ecs);
void ecs_command_queue_deinit(EcsCommandQueue *queue);
void ecs_command_queue_register_system(EcsCommandQueue *queue,
                                       const EcsSystemDescriptor *system);
EcsId ecs_command_queue_create_entity(EcsCommandQueue *queue);
EcsId ecs_command_queue_import_glb(EcsCommandQueue *queue, Assets *assets,
                                   const char *glb_path);
void ecs_command_queue_insert_component_(EcsCommandQueue *queue, EcsId entity,
                                         char *component_name,
                                         size_t component_size,
                                         void *component_data);
void ecs_command_queue_insert_tag_component_(EcsCommandQueue *queue,
                                             EcsId entity,
                                             char *component_name);
void ecs_command_queue_insert_relationship_(EcsCommandQueue *queue,
                                            EcsId source_entity,
                                            char *relationship_name,
                                            EcsId target_entity);
void ecs_command_queue_finish(Ecs *ecs, EcsCommandQueue *queue);
#define ecs_command_queue_insert_component(queue, entity, component_type, ...) \
  ecs_command_queue_insert_component_(queue, entity, #component_type,          \
                                      sizeof(component_type),                  \
                                      &(component_type)__VA_ARGS__)
#define ecs_command_queue_insert_component_with_ptr(queue, entity,             \
                                                    component_type, ptr)       \
  ecs_command_queue_insert_component_(queue, entity, #component_type,          \
                                      sizeof(component_type), ptr)
#define ecs_command_queue_insert_tag_component(queue, entity, component_type)  \
  ecs_command_queue_insert_tag_component_(queue, entity, #component_type)
#define ecs_command_queue_insert_relationship(queue, source, relationship,     \
                                              target)                          \
  ecs_command_queue_insert_relationship_(queue, source, #relationship, target)

void ecs_default_init_system(EcsCommandQueue *queue, EcsQueryIt *it);

DECL_VEC(EcsSystem, EcsSystemVec)

struct Ecs {
  Allocator *allocator;
  HashTable component_stores;
  HashTable relationship_stores;
  size_t entity_count;
  size_t reserved_entity_count;
  EcsSystemVec systems;
  EcsCommandQueue command_queue;
};

void ecs_init(Allocator *allocator, Ecs *ecs, EcsSystemFn init_system,
              void *system_context);
void ecs_deinit(Ecs *ecs);
void ecs_register_system(Ecs *ecs,
                         const EcsSystemDescriptor *system_descriptor);
void ecs_run_systems(Ecs *ecs, const void *system_context);
void ecs_process_command_queue(Ecs *ecs);
EcsId ecs_reserve_entity(Ecs *ecs);
EcsId ecs_create_entity(Ecs *ecs);
size_t ecs_get_entity_count(const Ecs *ecs);
void ecs_insert_component_(Ecs *ecs, EcsId entity_id, char *component_name,
                           size_t component_size, const void *data);
void ecs_insert_relationship_(Ecs *ecs, EcsId source, char *relationship_name,
                              EcsId target);
bool ecs_has_component_(const Ecs *ecs, EcsId entity_id,
                        const char *component_name);
void *ecs_get_component_(const Ecs *ecs, EcsId entity_id,
                         const char *component_name);
HashSet *ecs_get_relationship_sources_(const Ecs *ecs,
                                       const char *relationship_name,
                                       EcsId target);
HashSet *ecs_get_relationship_targets_(const Ecs *ecs, EcsId source,
                                       const char *relationship_name);
size_t ecs_count_matching(const Ecs *ecs, const EcsQuery *query);
EcsQueryIt ecs_query(const Ecs *ecs, const EcsQuery *query);
bool ecs_query_is_matching(const Ecs *ecs, const EcsQuery *query,
                           EcsId entity_id);
bool ecs_query_it_next(EcsQueryIt *it);
void *ecs_query_it_get_(const EcsQueryIt *it, size_t component);
EcsId ecs_query_it_entity_id(const EcsQueryIt *it);
void ecs_query_it_deinit(EcsQueryIt *it);
uint64_t ecs_id_hash_fn(const void *ecs_id);
bool ecs_id_eq_fn(const void *a, const void *b);
void ecs_id_dctor_fn(Allocator *allocator, void *ecs_id);

#define ecs_insert_component_with_ptr(ecs, entity_id, component_type, ptr)     \
  ecs_insert_component_(ecs, entity_id, #component_type,                       \
                        sizeof(component_type), ptr)
#define ecs_insert_component(ecs, entity_id, component_type, ...)              \
  ecs_insert_component_(ecs, entity_id, #component_type,                       \
                        sizeof(component_type), &(component_type)__VA_ARGS__)
#define ecs_insert_relationship(ecs, source, relationship_type, target)        \
  ecs_insert_relationship_(ecs, source, #relationship_type, target)

#define ecs_get_relationship_sources(ecs, relationship_type, target)           \
  ecs_get_relationship_sources_(ecs, #relationship_type, target)
#define ecs_get_relationship_targets(ecs, source, relationship_type)           \
  ecs_get_relationship_targets_(ecs, source, #relationship_type)

#define ecs_component_id(component_type) #component_type

#define ecs_has_component(ecs, entity_id, component_type)                      \
  ecs_has_component_(ecs, entity_id, ecs_component_id(component_type))
#define ecs_get_component(ecs, entity_id, component_type)                      \
  (component_type *)ecs_get_component_(ecs, entity_id,                         \
                                       ecs_component_id(component_type))

#define ecs_query_it_get(it, component_type, index)                            \
  (component_type *)ecs_query_it_get_(it, index)

#endif // CUTTERENG_ECS_ECS_H
