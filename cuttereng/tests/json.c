#include "test.h"
#include <json.h>

static void parse_true_literal() {
  const char json[] = "true";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_NOT_NULL(parsed_json);
  ASSERT_EQ(parsed_json->type, JSON_BOOLEAN);
  ASSERT(parsed_json->boolean);
  json_destroy(parsed_json);
}

static void parse_false_literal() {
  const char json[] = "false";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_NOT_NULL(parsed_json);
  ASSERT_EQ(parsed_json->type, JSON_BOOLEAN);
  ASSERT(!parsed_json->boolean);
  json_destroy(parsed_json);
}

static void parse_null_literal() {
  const char json[] = "null";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_NOT_NULL(parsed_json);
  ASSERT_EQ(parsed_json->type, JSON_NULL);
  json_destroy(parsed_json);
}

static void parse_number_integer() {
  const char json[] = "1996";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_NOT_NULL(parsed_json);
  ASSERT_EQ(parsed_json->type, JSON_NUMBER);
  ASSERT_FLOAT_EQ(parsed_json->number, 1996, 0.1);
  json_destroy(parsed_json);
}

static void parse_number_integer_with_whitespaces() {
  const char json[] = "   \t\n\r 1996   \n";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_NOT_NULL(parsed_json);
  ASSERT_EQ(parsed_json->type, JSON_NUMBER);
  ASSERT_FLOAT_EQ(parsed_json->number, 1996, 0.1);
  json_destroy(parsed_json);
}

static void parse_number_integer_zero() {
  const char json[] = "0";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_NOT_NULL(parsed_json);
  ASSERT_EQ(parsed_json->type, JSON_NUMBER);
  ASSERT_FLOAT_EQ(parsed_json->number, 0.0, 0.1);
  json_destroy(parsed_json);
}

static void parse_number_fractional() {
  const char json[] = "19.96";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_NOT_NULL(parsed_json);
  ASSERT_EQ(parsed_json->type, JSON_NUMBER);
  ASSERT_FLOAT_EQ(parsed_json->number, 19.96, 0.1);
  json_destroy(parsed_json);
}

static void parse_number_exponent() {
  const char json[] = "19.96e2";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_NOT_NULL(parsed_json);
  ASSERT_EQ(parsed_json->type, JSON_NUMBER);
  ASSERT_FLOAT_EQ(parsed_json->number, 1996, 0.1);
  json_destroy(parsed_json);
}

static void parse_ascii_string() {
  const char json[] = "\"Bonjour\"";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_NOT_NULL(parsed_json);
  ASSERT_EQ(parsed_json->type, JSON_STRING);
  ASSERT(strcmp(parsed_json->string, "Bonjour") == 0);
  json_destroy(parsed_json);
}

static void parse_string_with_escape_sequence() {
  const char json[] = "\"Bonjour\\\"Test\"";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_NOT_NULL(parsed_json);
  ASSERT_EQ(parsed_json->type, JSON_STRING);
  ASSERT(strcmp(parsed_json->string, "Bonjour\"Test") == 0);
  json_destroy(parsed_json);
}

static void parse_string_with_unicode_sequence() {
  const char json[] = "\"Bonjour\\u20AC\"";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_NOT_NULL(parsed_json);
  ASSERT_EQ(parsed_json->type, JSON_STRING);
  ASSERT(strcmp(parsed_json->string, "Bonjour\xe2\x82\xac") == 0);
  json_destroy(parsed_json);
}
static void parse_string_with_unicode_sequence_with_surrogate_pair() {
  const char json[] = "\"Bonjour\\uD83D\\uDE4A\"";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_NOT_NULL(parsed_json);
  ASSERT_EQ(parsed_json->type, JSON_STRING);
  ASSERT(strcmp(parsed_json->string, "Bonjour\xf0\x9f\x99\x8a") == 0);
  json_destroy(parsed_json);
}

static void parse_array() {
  const char json[] = "[1, 2, 4, 5]";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_NOT_NULL(parsed_json);
  ASSERT_EQ(parsed_json->type, JSON_ARRAY);
  ASSERT_EQ(parsed_json->array[0]->type, JSON_NUMBER);
  ASSERT_FLOAT_EQ(parsed_json->array[0]->number, 1.0, 0.1);
  ASSERT_EQ(parsed_json->array[1]->type, JSON_NUMBER);
  ASSERT_FLOAT_EQ(parsed_json->array[1]->number, 2.0, 0.1);
  ASSERT_EQ(parsed_json->array[2]->type, JSON_NUMBER);
  ASSERT_FLOAT_EQ(parsed_json->array[2]->number, 4.0, 0.1);
  ASSERT_EQ(parsed_json->array[3]->type, JSON_NUMBER);
  ASSERT_FLOAT_EQ(parsed_json->array[3]->number, 5.0, 0.1);
  ASSERT_NULL(parsed_json->array[4]);
  json_destroy(parsed_json);
}

static void parse_array_with_reallocation() {
  const char json[] =
      "[1, 2, 4, 5, 1, 2, 3, 4, 5, 6, 3, 1, 2, 4, 5, 6, 2, 12, 34, 5432, 234]";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_NOT_NULL(parsed_json);
  ASSERT_EQ(parsed_json->type, JSON_ARRAY);
  for (size_t i = 0; i < 21; i++) {
    ASSERT_NOT_NULL(parsed_json->array[i]);
  }
  ASSERT_NULL(parsed_json->array[21]);
  json_destroy(parsed_json);
}

static void parse_heterogeneous_array() {
  const char json[] = "[1, \"this is a string\", 4, []]";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_NOT_NULL(parsed_json);
  ASSERT_EQ(parsed_json->type, JSON_ARRAY);
  ASSERT_EQ(parsed_json->array[0]->type, JSON_NUMBER);
  ASSERT_FLOAT_EQ(parsed_json->array[0]->number, 1.0, 0.1);
  ASSERT_EQ(parsed_json->array[1]->type, JSON_STRING);
  ASSERT(strcmp(parsed_json->array[1]->string, "this is a string") == 0);
  ASSERT_EQ(parsed_json->array[2]->type, JSON_NUMBER);
  ASSERT_FLOAT_EQ(parsed_json->array[2]->number, 4.0, 0.1);
  ASSERT_EQ(parsed_json->array[3]->type, JSON_ARRAY);
  ASSERT_NULL(parsed_json->array[4]);
  json_destroy(parsed_json);
}

static void parse_empty_object() {
  const char json[] = "{}";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_EQ(parsed_json->type, JSON_OBJECT);
  json_destroy(parsed_json);
}

static void parse_object_with_single_attribute() {
  const char json[] = "{\"some_field\": \"some_value\"}";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_EQ(parsed_json->type, JSON_OBJECT);
  ASSERT_NOT_NULL(json_object_get(parsed_json->object, "some_field"));
  json_destroy(parsed_json);
}

static void parse_object_with_multiple_attributes() {
  const char json[] =
      "{\"some_field\": \"some_value\", \"bla\":\"blu\", \"bli\":\"blo\"}";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_EQ(parsed_json->type, JSON_OBJECT);
  ASSERT_NOT_NULL(json_object_get(parsed_json->object, "some_field"));
  json_destroy(parsed_json);
}

static void parse_object_with_a_ton_of_attributes() {
  const char json[] =
      "{\"property0\":0,\"property1\":1,\"property2\":2,\"property3\":3,"
      "\"property4\":4,\"property5\":5,\"property6\":6,\"property7\":7,"
      "\"property8\":"
      "8,\"property9\":9,\"property10\":10,\"property11\":11,\"property12\":12,"
      "\"property13\":13,\"property14\":14,\"property15\":15,\"property16\":16,"
      "\"property17\":17,\"property18\":18,\"property19\":19,\"property20\":20,"
      "\"property21\":21,\"property22\":22,\"property23\":23,\"property24\":24,"
      "\"property25\":25,\"property26\":26,\"property27\":27,\"property28\":28,"
      "\"property29\":29,\"property30\":30,\"property31\":31,\"property32\":32,"
      "\"property33\":33,\"property34\":34,\"property35\":35,\"property36\":36,"
      "\"property37\":37,\"property38\":38,\"property39\":39,\"property40\":40,"
      "\"property41\":41,\"property42\":42,\"property43\":43,\"property44\":44,"
      "\"property45\":45,\"property46\":46,\"property47\":47,\"property48\":48,"
      "\"property49\":49,\"property50\":50,\"property51\":51,\"property52\":52,"
      "\"property53\":53,\"property54\":54,\"property55\":55,\"property56\":56,"
      "\"property57\":57,\"property58\":58,\"property59\":59,\"property60\":60,"
      "\"property61\":61,\"property62\":62,\"property63\":63,\"property64\":64,"
      "\"property65\":65,\"property66\":66,\"property67\":67,\"property68\":68,"
      "\"property69\":69,\"property70\":70,\"property71\":71,\"property72\":72,"
      "\"property73\":73,\"property74\":74,\"property75\":75,\"property76\":76,"
      "\"property77\":77,\"property78\":78,\"property79\":79,\"property80\":80,"
      "\"property81\":81,\"property82\":82,\"property83\":83,\"property84\":84,"
      "\"property85\":85,\"property86\":86,\"property87\":87,\"property88\":88,"
      "\"property89\":89,\"property90\":90,\"property91\":91,\"property92\":92,"
      "\"property93\":93,\"property94\":94,\"property95\":95,\"property96\":96,"
      "\"property97\":97,\"property98\":98,\"property99\":99,\"property100\":"
      "100}";
  json_value *parsed_json = json_parse_from_str(json);
  ASSERT_EQ(parsed_json->type, JSON_OBJECT);
  ASSERT_NOT_NULL(json_object_get(parsed_json->object, "property97"));
  ASSERT_FLOAT_EQ(json_object_get(parsed_json->object, "property97")->number,
                  97.0, 0.1);
  json_destroy(parsed_json);
}

TEST_SUITE(TEST(parse_true_literal), TEST(parse_false_literal),
           TEST(parse_null_literal), TEST(parse_number_integer),
           TEST(parse_number_integer_with_whitespaces),
           TEST(parse_number_integer_zero), TEST(parse_number_fractional),
           TEST(parse_number_exponent), TEST(parse_ascii_string),
           TEST(parse_string_with_escape_sequence),
           TEST(parse_string_with_unicode_sequence),
           TEST(parse_string_with_unicode_sequence_with_surrogate_pair),
           TEST(parse_array), TEST(parse_array_with_reallocation),
           TEST(parse_heterogeneous_array), TEST(parse_empty_object),
           TEST(parse_object_with_single_attribute),
           TEST(parse_object_with_multiple_attributes),
           TEST(parse_object_with_a_ton_of_attributes));
