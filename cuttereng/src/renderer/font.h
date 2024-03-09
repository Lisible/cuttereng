#ifndef CUTTERENG_RENDERER_FONT_H
#define CUTTERENG_RENDERER_FONT_H

#include "../asset.h"
#include "../common.h"
#include "../memory.h"

#define MAX_FONT_GLYPH_COUNT 29

typedef struct {
  u32 top_left_x;
  u32 top_left_y;
  u32 bottom_right_x;
  u32 bottom_right_y;
} GlyphRectangle;

typedef struct {
  char glyph;
  GlyphRectangle rectangle;
} Glyph;

typedef struct {
  Glyph glyphs[MAX_FONT_GLYPH_COUNT];
  char *atlas;
  size_t glyph_count;
} BitmapFont;

void bitmap_font_deinitializer_fn(Allocator *allocator, void *ptr);
extern AssetDeinitializer bitmap_font_deinitializer;

extern AssetLoader bitmap_font_loader;
void *bitmap_font_loader_fn(Allocator *allocator, Assets *assets,
                            const char *path);

#endif // CUTTERENG_RENDERER_FONT_H
