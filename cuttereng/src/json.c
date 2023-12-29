#include "json.h"
#include "log.h"
#include "src/assert.h"
#include "src/hash.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOKEN_ARRAY_BEGIN '['
#define TOKEN_ARRAY_END ']'
#define TOKEN_OBJECT_BEGIN '{'
#define TOKEN_OBJECT_END '}'
#define TOKEN_COLON ':'
#define TOKEN_COMMA ','
#define TOKEN_TRUE "true"
#define TOKEN_FALSE "false"
#define TOKEN_NULL "null"
#define TOKEN_MINUS '-'
#define TOKEN_DOUBLE_QUOTE '"'

#define CODEPOINT_WHITESPACE 0x0020
#define CODEPOINT_CHARACTER_TABULATION 0x0009
#define CODEPOINT_LINE_FEED 0x000A
#define CODEPOINT_CARRIAGE_RETURN 0x000D

typedef struct {
  Allocator *allocator;
  const char *str;
  size_t len;
  size_t index;
  size_t line;
  size_t column;
} ParsingContext;

void advance(ParsingContext *ctx, size_t count);
Json *parse_element(ParsingContext *ctx);
Json *parse_value(ParsingContext *ctx);
bool parse_object(ParsingContext *ctx, Json *output_value);
bool parse_array(ParsingContext *ctx, Json *output_value);
void parse_number(ParsingContext *ctx, Json *output_value);
bool parse_string(ParsingContext *ctx, Json *output_value);
void eat_character(ParsingContext *ctx, char expected);
void eat_whitespaces(ParsingContext *ctx);
bool is_character(char c);
bool is_escapable_character(char c);
bool is_digit(char c);
bool is_non_zero_digit(char c);
char current_character(ParsingContext *ctx);
char next_character(ParsingContext *ctx);
Json *json_create(Allocator *allocator);
const char *json_object_set(JsonObject *object, char *key, Json *value);

Json *json_parse_from_str(Allocator *allocator, const char *str) {
  ASSERT(str != NULL);
  ParsingContext ctx = {.allocator = allocator,
                        .str = str,
                        .len = strlen(str),
                        .index = 0,
                        .line = 0,
                        .column = 0};
  return parse_element(&ctx);
}

Json *parse_element(ParsingContext *ctx) {
  ASSERT(ctx != NULL);
  eat_whitespaces(ctx);
  Json *value = parse_value(ctx);
  eat_whitespaces(ctx);
  return value;
}

Json *parse_value(ParsingContext *ctx) {
  ASSERT(ctx != NULL);
  Json *value = json_create(ctx->allocator);
  if (!value) {
    LOG_ERROR("json allocation failed");
    goto err;
  }

  if (strcmp(&ctx->str[ctx->index], TOKEN_TRUE) == 0) {
    value->type = JSON_BOOLEAN;
    value->boolean = true;
    advance(ctx, strlen(TOKEN_TRUE));
  } else if (strcmp(&ctx->str[ctx->index], TOKEN_FALSE) == 0) {
    value->type = JSON_BOOLEAN;
    value->boolean = false;
    advance(ctx, strlen(TOKEN_FALSE));
  } else if (strcmp(&ctx->str[ctx->index], TOKEN_NULL) == 0) {
    value->type = JSON_NULL;
    advance(ctx, strlen(TOKEN_NULL));
  } else if (current_character(ctx) == TOKEN_MINUS ||
             is_digit(current_character(ctx))) {
    parse_number(ctx, value);
  } else if (current_character(ctx) == TOKEN_DOUBLE_QUOTE) {
    if (!parse_string(ctx, value)) {
      JSON_LOG_PARSE_ERROR(ctx, "couldn't parse json string");
      goto err_2;
    }
  } else if (current_character(ctx) == TOKEN_ARRAY_BEGIN) {
    if (!parse_array(ctx, value)) {
      JSON_LOG_PARSE_ERROR(ctx, "couldn't parse json array");
      goto err_2;
    }
  } else if (current_character(ctx) == TOKEN_OBJECT_BEGIN) {
    if (!parse_object(ctx, value)) {
      JSON_LOG_PARSE_ERROR(ctx, "couldn't parse json object");
      goto err_2;
    }
  } else {
    JSON_LOG_PARSE_ERROR(ctx, "unexpected token: %c", current_character(ctx));
    goto err_2;
  }

  return value;

err_2:
  free(value);
err:
  return NULL;
}

typedef struct {
  const char *name;
  Json *value;
} JsonObjectProperty;

DECL_HASH_TABLE(Json *, HashTableJson)
DEF_HASH_TABLE(Json *, HashTableJson, json_destroy)

struct JsonObject {
  HashTableJson *hash_table;
};

JsonObject *json_object_create(Allocator *allocator) {
  JsonObject *object = allocator_allocate(allocator, sizeof(JsonObject));
  if (!object) {
    LOG_ERROR("json object allocation failed");
    goto err;
  }

  object->hash_table = HashTableJson_create(allocator, 16);
  if (!object->hash_table)
    goto cleanup;

  return object;

cleanup:
  free(object);
err:
  return NULL;
}

bool parse_object(ParsingContext *ctx, Json *output_value) {
  ASSERT(ctx != NULL);
  ASSERT(output_value != NULL);
  JsonObject *object = json_object_create(ctx->allocator);
  if (!object) {
    return false;
  }

  eat_character(ctx, TOKEN_OBJECT_BEGIN);

  while (current_character(ctx) != TOKEN_OBJECT_END) {
    eat_whitespaces(ctx);

    Json *name = json_create(ctx->allocator);
    if (!name) {
      return false;
    }
    parse_string(ctx, name);

    eat_whitespaces(ctx);
    eat_character(ctx, TOKEN_COLON);
    eat_whitespaces(ctx);

    Json *value = parse_value(ctx);
    if (!value) {
      return false;
    }
    json_object_set(object, name->string, value);
    json_destroy(ctx->allocator, name);

    eat_whitespaces(ctx);
    if (current_character(ctx) == TOKEN_COMMA) {
      eat_character(ctx, TOKEN_COMMA);
    }
  }

  eat_character(ctx, TOKEN_OBJECT_END);
  output_value->type = JSON_OBJECT;
  output_value->object = object;
  return true;
}

bool parse_array(ParsingContext *ctx, Json *output_value) {
  ASSERT(ctx != NULL);
  ASSERT(output_value != NULL);
  static const size_t MINIMUM_ARRAY_CAPACITY = 16;

  eat_character(ctx, TOKEN_ARRAY_BEGIN);
  size_t capacity = MINIMUM_ARRAY_CAPACITY;

  Json **array =
      allocator_allocate(ctx->allocator, (capacity + 1) * sizeof(Json *));
  if (!array) {
    LOG_ERROR("memory allocation failed");
    goto err;
  }

  size_t length = 0;
  while (current_character(ctx) != TOKEN_ARRAY_END) {
    eat_whitespaces(ctx);
    if (length == capacity) {
      size_t old_capacity = capacity;
      capacity *= 2;
      array = allocator_reallocate(ctx->allocator, array,
                                   (old_capacity + 1) * sizeof(Json *),
                                   (capacity + 1) * sizeof(Json *));
      if (!array) {
        LOG_ERROR("memory reallocation failed");
        goto err;
      }
    }

    array[length] = parse_value(ctx);
    if (!array[length]) {
      goto cleanup;
    }
    length++;

    eat_whitespaces(ctx);

    if (current_character(ctx) == TOKEN_COMMA) {
      eat_character(ctx, TOKEN_COMMA);
    }

    eat_whitespaces(ctx);
  }

  array[length] = NULL;

  eat_character(ctx, TOKEN_ARRAY_END);

  output_value->type = JSON_ARRAY;
  output_value->array = array;

  return true;

cleanup:
  free(array);

err:
  return false;
}

size_t estimate_string_size(const char *str);
uint16_t parse_unicode_hex(ParsingContext *ctx);
size_t write_utf8_from_code_point(char *string, size_t current_index,
                                  uint32_t code_point);
bool is_utf16_leading_surrogate(uint32_t code_point);
uint32_t code_point_from_surrogates(uint16_t leading_surrogate,
                                    uint16_t trailing_surrogate);
bool parse_string(ParsingContext *ctx, Json *output_value) {
  ASSERT(ctx != NULL);
  ASSERT(output_value != NULL);
  eat_character(ctx, TOKEN_DOUBLE_QUOTE);
  size_t estimated_string_size = estimate_string_size(&ctx->str[ctx->index]);
  char *string =
      allocator_allocate(ctx->allocator, estimated_string_size * sizeof(char));
  if (!string) {
    LOG_ERROR("json string allocation failed");
    goto err;
  }

  memset(string, 0, estimated_string_size * sizeof(char));

  size_t string_index = 0;
  while (current_character(ctx) != TOKEN_DOUBLE_QUOTE) {
    if (current_character(ctx) == '\\') {
      eat_character(ctx, '\\');
      if (current_character(ctx) == 'u') {
        eat_character(ctx, 'u');
        uint32_t code_point = parse_unicode_hex(ctx);
        if (is_utf16_leading_surrogate(code_point)) {
          uint16_t leading_surrogate = code_point;
          eat_character(ctx, '\\');
          eat_character(ctx, 'u');
          uint16_t trailing_surrogate = parse_unicode_hex(ctx);
          code_point =
              code_point_from_surrogates(leading_surrogate, trailing_surrogate);
        }

        string_index +=
            write_utf8_from_code_point(string, string_index, code_point);
      } else {
        if (!is_escapable_character(current_character(ctx))) {
          JSON_LOG_PARSE_ERROR(ctx, "character %c is not escapable",
                               current_character(ctx));
          goto err_2;
        }

        string[string_index] = current_character(ctx);
        string_index++;
        advance(ctx, 1);
      }
    } else {
      string[string_index] = current_character(ctx);
      string_index++;
      advance(ctx, 1);
    }
  }

  eat_character(ctx, TOKEN_DOUBLE_QUOTE);
  output_value->type = JSON_STRING;
  output_value->string = string;
  return true;

err_2:
  free(string);

err:
  return false;
}

bool is_utf16_leading_surrogate(uint32_t code_point) {
  return code_point >= 0xD800 && code_point <= 0xDBFF;
}

uint32_t code_point_from_surrogates(uint16_t leading_surrogate,
                                    uint16_t trailing_surrogate) {
  uint32_t X = (leading_surrogate & ((1 << 6) - 1)) << 10 |
               (trailing_surrogate & ((1 << 10) - 1));
  uint32_t W = (leading_surrogate >> 6) & ((1 << 5) - 1);
  uint32_t U = W + 1;
  return U << 16 | X;
}
size_t write_utf8_from_code_point(char *string, size_t current_index,
                                  uint32_t code_point) {
  ASSERT(string != NULL);
  if (code_point <= 0x7f) {
    string[current_index] = code_point;
    return 1;
  }

  if (code_point <= 0x7ff) {
    string[current_index] = 0xe0 | (code_point >> 6);
    string[current_index + 1] = 0x80 | (code_point & 0x3f);
    return 2;
  }

  if (code_point <= 0xffff) {
    string[current_index] = 0xe0 | (code_point >> 12);
    string[current_index + 1] = 0x80 | ((code_point >> 6) & 0x3f);
    string[current_index + 2] = 0x80 | (code_point & 0x3f);
    return 3;
  }

  string[current_index] = 0xf0 | (code_point >> 18);
  string[current_index + 1] = 0x80 | ((code_point >> 12) & 0x3f);
  string[current_index + 2] = 0x80 | ((code_point >> 6) & 0x3f);
  string[current_index + 3] = 0x80 | (code_point & 0x3f);
  return 4;
}

uint16_t parse_unicode_hex(ParsingContext *ctx) {
  ASSERT(ctx != NULL);
  uint16_t parsed_codepoint = 0;
  for (int i = 3; i >= 0; i--) {
    char v = current_character(ctx);
    int n;
    if (v >= 'a' && v <= 'f') {
      n = v - 'a' + 10;
    } else if (v >= 'A' && v <= 'F') {
      n = v - 'A' + 10;
    } else {
      n = v - '0';
    }

    parsed_codepoint = (parsed_codepoint << 4) + n;
    advance(ctx, 1);
  }

  return parsed_codepoint;
}

size_t estimate_string_size(const char *str) {
  ASSERT(str != NULL);
  size_t i = 0;
  size_t estimated_str_length = 0;
  while (str[i] != TOKEN_DOUBLE_QUOTE) {
    if (str[i] == '\\') {
      if (str[i + 1] == 'u') {
        estimated_str_length += 4;
        i += 5;
      } else {
        estimated_str_length++;
        i += 2;
      }
    } else {
      estimated_str_length++;
      i++;
    }
  }
  estimated_str_length++;

  return estimated_str_length;
}

void parse_number(ParsingContext *ctx, Json *output_value) {
  ASSERT(ctx != NULL);
  ASSERT(output_value != NULL);
  int sign = 1;
  if (current_character(ctx) == TOKEN_MINUS) {
    sign = -1;
    advance(ctx, 1);
  }

  double number = 0;
  if (is_non_zero_digit(current_character(ctx))) {
    while (is_digit(current_character(ctx))) {
      number = number * 10 + (current_character(ctx) - '0');
      advance(ctx, 1);
    };
  } else if (current_character(ctx) == '0') {
    advance(ctx, 1);
  }

  if (current_character(ctx) == '.') {
    size_t nth_fractional_digit = 0;
    advance(ctx, 1);

    while (is_digit(current_character(ctx))) {
      number = number + ((current_character(ctx) - '0') /
                         pow(10.0, nth_fractional_digit + 1));
      nth_fractional_digit++;
      advance(ctx, 1);
    }
  }

  int exponent = 0;
  if (current_character(ctx) == 'e' || current_character(ctx) == 'E') {
    advance(ctx, 1);
    while (is_digit(current_character(ctx))) {
      exponent += exponent * 10 + (current_character(ctx) - '0');
      advance(ctx, 1);
    }
  }

  output_value->type = JSON_NUMBER;
  output_value->number = sign * (number * pow(10, exponent));
}

void json_cleanup(Allocator *allocator, Json *value) {
  ASSERT(value != NULL);
  if (value->type == JSON_STRING) {
    allocator_free(allocator, value->string);
    value->string = NULL;
  } else if (value->type == JSON_ARRAY) {
    size_t i = 0;
    while (value->array[i] != NULL) {
      json_destroy(allocator, value->array[i]);
      i++;
    }

    allocator_free(allocator, value->array);
    value->array = NULL;
  } else if (value->type == JSON_OBJECT) {
    HashTableJson_destroy(value->object->hash_table);
    allocator_free(allocator, value->object);
    value->object = NULL;
  }
}

void json_destroy(Allocator *allocator, Json *value) {
  if (!value)
    return;

  json_cleanup(allocator, value);
  allocator_free(allocator, value);
}

void json_destroy_without_cleanup(Allocator *allocator, Json *value) {
  if (!value)
    return;

  allocator_free(allocator, value);
}

void eat_character(ParsingContext *ctx, char expected) {
  ASSERT(ctx != NULL);
  ASSERT(current_character(ctx) == expected);
  advance(ctx, 1);
}

bool is_whitespace(char c) {
  return c == CODEPOINT_WHITESPACE || c == CODEPOINT_CHARACTER_TABULATION ||
         c == CODEPOINT_CARRIAGE_RETURN || c == CODEPOINT_LINE_FEED;
}

void eat_whitespaces(ParsingContext *ctx) {
  ASSERT(ctx != NULL);
  while (ctx->index < ctx->len && is_whitespace(current_character(ctx))) {

    if (current_character(ctx) == CODEPOINT_LINE_FEED) {
      ctx->line++;
      ctx->column = 0;
    }

    ctx->index++;
  }
}

void advance(ParsingContext *ctx, size_t count) {
  ASSERT(ctx != NULL);
  ctx->index += count;
  ctx->column += count;
}

char current_character(ParsingContext *ctx) {
  ASSERT(ctx != NULL);
  return ctx->str[ctx->index];
}
char next_character(ParsingContext *ctx) {
  ASSERT(ctx != NULL);
  return ctx->str[ctx->index + 1];
}

bool is_character(char c) { return !is_escapable_character(c); }

bool is_escapable_character(char c) {
  return c == '\"' || c == '\\' || c == '/' || c == '\b' || c == '\f' ||
         c == '\n' || c == '\r' || c == '\t';
}
bool is_digit(char c) { return c >= '0' && c <= '9'; }
bool is_non_zero_digit(char c) { return is_digit(c) && c != '0'; }

Json *json_create(Allocator *allocator) {
  return allocator_allocate(allocator, (sizeof(Json)));
}

Json *json_object_get(const JsonObject *object, const char *key) {
  ASSERT(object != NULL);
  ASSERT(key != NULL);
  return HashTableJson_get(object->hash_table, key);
}

const char *json_object_set(JsonObject *object, char *key, Json *value) {
  ASSERT(object != NULL);
  ASSERT(key != NULL);
  ASSERT(value != NULL);
  return HashTableJson_set(object->hash_table, key, value);
}

void json_object_steal(JsonObject *object, const char *key) {
  ASSERT(object != NULL);
  ASSERT(key != NULL);
  HashTableJson_steal(object->hash_table, key);
}
