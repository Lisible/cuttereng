#ifndef CUTTERENG_ECS_H
#define CUTTERENG_ECS_H

#include "common.h"
#include "hash.h"

typedef size_t ecs_id;
typedef struct ecs ecs;
struct ecs {
  hash_table *component_stores;
  size_t entity_count;
};

void ecs_init(ecs *ecs);
void ecs_deinit(ecs *ecs);
ecs_id ecs_create_entity(ecs *ecs);
size_t ecs_get_entity_count(ecs *ecs);
void ecs_insert_component(ecs *ecs, ecs_id entity_id, char *component_name,
                          size_t component_size, void *data);
bool ecs_has_component(ecs *ecs, ecs_id entity_id, char *component_name);
void *ecs_get_component(ecs *ecs, ecs_id entity_id, char *component_name);

#define ecs_insert(ecs, entity_id, component_type, ...)                        \
  ecs_insert_component(ecs, entity_id, #component_type,                        \
                       sizeof(component_type), &(component_type)__VA_ARGS__)

#define ecs_has(ecs, entity_id, component_type)                                \
  ecs_has_component(ecs, entity_id, #component_type)
#define ecs_get(ecs, entity_id, component_type)                                \
  (component_type *)ecs_get_component(ecs, entity_id, #component_type)

#endif
