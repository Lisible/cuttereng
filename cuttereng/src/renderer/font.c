#include "font.h"
#include "../assert.h"
#include "../json.h"

AssetLoader bitmap_font_loader = {.fn = bitmap_font_loader_fn};
AssetDestructor bitmap_font_destructor = {.fn = bitmap_font_destructor_fn};
void bitmap_font_destructor_fn(Allocator *allocator, void *ptr) {
  ASSERT(allocator != NULL);
  BitmapFont *font = ptr;
  if (!font) {
    return;
  }
  allocator_free(allocator, font->atlas);
  // allocator_free(allocator, font);
}

void *bitmap_font_loader_fn(Allocator *allocator, Assets *assets,
                            const char *path) {
  BitmapFont *font = allocator_allocate(allocator, sizeof(BitmapFont));
  if (!font) {
    LOG_ERROR("Couldn't allocate font");
    goto err;
  }

  char *font_file_content = asset_read_file_to_string(allocator, path, NULL);
  if (!font_file_content) {
    LOG_ERROR("Couldn't read font file: %s", path);
    goto cleanup_font;
  }

  Json *font_json = json_parse_from_str(allocator, font_file_content);
  if (!font_json) {
    LOG_ERROR("Couldn't parse font file: %s", path);
    goto cleanup_file_content_str;
  }

  if (font_json->type != JSON_OBJECT) {
    LOG_ERROR("Font file %s is not a JSON object", path);
    goto cleanup_font_json;
  }

  Json *atlas_json = json_object_get(font_json->object, "atlas");
  if (!atlas_json) {
    LOG_ERROR("atlas property missing from font file: %s", path);
    goto cleanup_font_json;
  }

  if (atlas_json->type != JSON_STRING) {
    LOG_ERROR("atlas property of font file %s is not a string", path);
    goto cleanup_font_json;
  }

  char *atlas = memory_clone_string(allocator, atlas_json->string);
  if (!atlas) {
    LOG_ERROR("Couldn't allocate font->atlas");
    goto cleanup_font_json;
  }

  Json *glyphs_json = json_object_get(font_json->object, "glyphs");
  if (!glyphs_json) {
    LOG_ERROR("glyphs property missing from font file: %s", path);
    goto cleanup_atlas_str;
  }

  if (glyphs_json->type != JSON_ARRAY) {
    LOG_ERROR("glyphs property of font file: %s is not an array", path);
    goto cleanup_atlas_str;
  }

  font->glyph_count = 0;
  for (size_t glyph_index = 0;
       glyph_index < json_array_length(glyphs_json->array); glyph_index++) {
    Json *glyph_element_json =
        json_array_at(glyphs_json->array, font->glyph_count);
    if (glyph_element_json->type != JSON_OBJECT) {
      LOG_ERROR("glyph element of font file: %s is not an object", path);
      goto cleanup_atlas_str;
    }

    Json *glyph_json = json_object_get(glyph_element_json->object, "glyph");
    if (!glyph_json) {
      LOG_ERROR("glyph property missing in glyph element from font file: %s",
                path);
      goto cleanup_atlas_str;
    }

    if (glyph_json->type != JSON_STRING) {
      LOG_ERROR(
          "glyph property of glyph element of font file: %s is not a string",
          path);
      goto cleanup_atlas_str;
    }
    Json *glyph_rect_json = json_object_get(glyph_element_json->object, "rect");
    if (!glyph_rect_json) {
      LOG_ERROR("rect property missing in glyph element from font file: %s",
                path);
      goto cleanup_atlas_str;
    }

    if (glyph_rect_json->type != JSON_ARRAY) {
      LOG_ERROR(
          "rect property of glyph element of font file: %s is not an array",
          path);
      goto cleanup_atlas_str;
    }

    Json *top_left_x_json = json_array_at(glyph_rect_json->array, 0);
    if (!top_left_x_json) {
      LOG_ERROR("top left x coordinate missing in glyph element's rect from "
                "font file: %s",
                path);
      goto cleanup_atlas_str;
    }

    if (top_left_x_json->type != JSON_NUMBER) {
      LOG_ERROR("top left x coordinate in glyph element's rect from "
                "font file: %s is not a number",
                path);
      goto cleanup_atlas_str;
    }
    Json *top_left_y_json = json_array_at(glyph_rect_json->array, 1);
    if (!top_left_y_json) {
      LOG_ERROR("top left y coordinate missing in glyph element's rect from "
                "font file: %s",
                path);
      goto cleanup_atlas_str;
    }

    if (top_left_y_json->type != JSON_NUMBER) {
      LOG_ERROR("top left y coordinate in glyph element's rect from "
                "font file: %s is not a number",
                path);
      goto cleanup_atlas_str;
    }
    Json *bottom_right_x_json = json_array_at(glyph_rect_json->array, 2);
    if (!bottom_right_x_json) {
      LOG_ERROR(
          "bottom right x coordinate missing in glyph element's rect from "
          "font file: %s",
          path);
      goto cleanup_atlas_str;
    }

    if (bottom_right_x_json->type != JSON_NUMBER) {
      LOG_ERROR("bottom right x coordinate in glyph element's rect from "
                "font file: %s is not a number",
                path);
      goto cleanup_atlas_str;
    }
    Json *bottom_right_y_json = json_array_at(glyph_rect_json->array, 3);
    if (!bottom_right_y_json) {
      LOG_ERROR(
          "bottom right y coordinate missing in glyph element's rect from "
          "font file: %s",
          path);
      goto cleanup_atlas_str;
    }

    if (bottom_right_y_json->type != JSON_NUMBER) {
      LOG_ERROR("bottom right y coordinate in glyph element's rect from "
                "font file: %s is not a number",
                path);
      goto cleanup_atlas_str;
    }

    u32 top_left_x = (int)top_left_x_json->number;
    u32 top_left_y = (int)top_left_y_json->number;
    u32 bottom_right_x = (int)bottom_right_x_json->number;
    u32 bottom_right_y = (int)bottom_right_y_json->number;

    // TODO this will not work for multibytes characters
    font->glyphs[font->glyph_count].glyph = glyph_json->string[0];
    font->glyphs[font->glyph_count].rectangle.top_left_x = top_left_x;
    font->glyphs[font->glyph_count].rectangle.top_left_y = top_left_y;
    font->glyphs[font->glyph_count].rectangle.bottom_right_x = bottom_right_x;
    font->glyphs[font->glyph_count].rectangle.bottom_right_y = bottom_right_y;

    font->glyph_count++;
    if (font->glyph_count > MAX_FONT_GLYPH_COUNT) {
      LOG_ERROR("Too many glyphs in font file: %s (>%d)", path,
                MAX_FONT_GLYPH_COUNT);
      goto cleanup_atlas_str;
    }
  }

  font->atlas = atlas;

  json_destroy(allocator, font_json);
  allocator_free(allocator, font_file_content);
  return font;
cleanup_atlas_str:
  allocator_free(allocator, atlas);
cleanup_font_json:
  json_destroy(allocator, font_json);
cleanup_file_content_str:
  allocator_free(allocator, font_file_content);
cleanup_font:
  allocator_free(allocator, font);
err:
  return NULL;
}
