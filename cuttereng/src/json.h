#ifndef CUTTERENG_JSON_H
#define CUTTERENG_JSON_H
#include <stdbool.h>
#include <stddef.h>

#define JSON_LOG_PARSE_ERROR(ctx, msg, ...)                                    \
  LOG_ERROR("%d:%d " msg, ctx->line, ctx->column, ##__VA_ARGS__)

typedef enum {
  JSON_OBJECT,
  JSON_ARRAY,
  JSON_STRING,
  JSON_NUMBER,
  JSON_BOOLEAN,
  JSON_NULL
} JsonValueType;

typedef struct JsonObject JsonObject;
typedef struct Json Json;
struct Json {
  JsonValueType type;
  union {
    JsonObject *object;
    Json **array;
    char *string;
    double number;
    bool boolean;
  };
};

Json *json_object_get(const JsonObject *object, const char *key);
void json_object_steal(JsonObject *object, const char *key);

/// Parses a json from a string
///
/// This value is dynamically allocated and must be freed using `json_free()`
/// @return The parsed json or NULL in case of error
Json *json_parse_from_str(const char *str);

/// Destroys a `json_value`
void json_destroy(Json *json_value);
/// Destroys a `json_value` but without cleaning up it's underlying data
void json_destroy_without_cleanup(Json *json_value);

#endif // CUTTERENG_JSON_H
