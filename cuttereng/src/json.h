#ifndef CUTTERENG_JSON_H
#define CUTTERENG_JSON_H
#include <stdbool.h>
#include <stddef.h>

#define JSON_LOG_PARSE_ERROR(ctx, msg, ...)                                    \
  LOG_ERROR("%d:%d " msg, ctx->line, ctx->column, ##__VA_ARGS__)

typedef enum json_value_type json_value_type;
typedef struct json json;
typedef struct json_object json_object;
json *json_object_get(json_object *object, char *key);
void json_object_steal(json_object *object, const char *key);

enum json_value_type {
  JSON_OBJECT,
  JSON_ARRAY,
  JSON_STRING,
  JSON_NUMBER,
  JSON_BOOLEAN,
  JSON_NULL
};

struct json {
  json_value_type type;
  union {
    json_object *object;
    json **array;
    char *string;
    double number;
    bool boolean;
  };
};

/// Parses a json from a string
///
/// This value is dynamically allocated and must be freed using `json_free()`
/// @return The parsed json or NULL in case of error
json *json_parse_from_str(const char *str);

/// Destroys a `json_value`
void json_destroy(json *json_value);
/// Destroys a `json_value` but without cleaning up it's underlying data
void json_destroy_without_cleanup(json *json_value);

#endif // CUTTERENG_JSON_H
