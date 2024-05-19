#ifndef CUTTERENG_JSON_H
#define CUTTERENG_JSON_H
#include <lisiblestd/memory.h>
#include <stdbool.h>

#define JSON_LOG_PARSE_ERROR(ctx, fmt, ...)                                    \
  LOG_ERROR("%zu:%zu " fmt, ctx->line, ctx->column, __VA_ARGS__)
#define JSON_LOG0_PARSE_ERROR(ctx, msg)                                        \
  LOG_ERROR("%zu:%zu %s", ctx->line, ctx->column, msg)

typedef enum {
  JSON_OBJECT,
  JSON_ARRAY,
  JSON_STRING,
  JSON_NUMBER,
  JSON_BOOLEAN,
  JSON_NULL
} JsonValueType;

typedef struct JsonObject JsonObject;
typedef struct JsonArray JsonArray;

typedef struct Json Json;
struct Json {
  JsonValueType type;
  union {
    JsonObject *object;
    JsonArray *array;
    char *string;
    double number;
    bool boolean;
  };
};

size_t json_array_length(const JsonArray *array);
Json *json_array_at(const JsonArray *array, size_t index);

/// Returns the JsonObject represented by the Json
//
// @return The associated JsonObject or NULL if json doesn't have the object
// type
JsonObject *json_as_object(const Json *json);
Json *json_object_get(const JsonObject *object, const char *key);
bool json_object_get_object(const JsonObject *object, const char *key,
                            JsonObject **out_object);
bool json_object_get_array(const JsonObject *object, const char *key,
                           JsonArray **out_array);
bool json_object_get_string(const JsonObject *object, const char *key,
                            char **out_str);
bool json_object_get_number(const JsonObject *object, const char *key,
                            double *out_number);
bool json_object_get_boolean(const JsonObject *object, const char *key,
                             bool *out_bool);
size_t json_object_get_key_count(const JsonObject *object);
char *json_object_get_key(const JsonObject *object, size_t index);

void json_object_steal(JsonObject *object, const char *key);

/// Parses a json from a string
///
/// This value is dynamically allocated and must be freed using `json_free()`
/// @return The parsed json or NULL in case of error
Json *json_parse_from_str(Allocator *allocator, const char *str);

/// Destroys a `json_value`
void json_destroy(Allocator *allocator, Json *json_value);
/// Destroys a `json_value` but without cleaning up it's underlying data
void json_destroy_without_cleanup(Allocator *allocator, Json *json_value);

#endif // CUTTERENG_JSON_H
