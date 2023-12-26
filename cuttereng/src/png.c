#include "png.h"
#include "assert.h"
#include "bytes.h"
#include "common.h"
#include "deflate.h"
#include "log.h"
#include "memory.h"
#include "vec.h"
#include "zlib.h"

#include <string.h>
static const int CHUNK_LENGTH_SIZE = 4;
static const int CHUNK_TYPE_SIZE = 4;
static const int CHUNK_CRC_SIZE = 4;

VEC_IMPL(u8, u8vec, 1024)

typedef struct {
  const u8 *datastream;
  size_t current_index;
} ParsingContext;

typedef enum {
  ChunkType_IHDR,
  ChunkType_IDAT,
  ChunkType_IEND,
  ChunkType_Unknown
} ChunkType;

typedef enum { ColourType_Truecolour = 2, ColourType_Unknown } ColourType;

typedef struct {
  size_t length;
  ChunkType type;
  u8 *data;
  u32 crc;
} Chunk;

bool parse_signature(ParsingContext *context) {
  ASSERT(context);
  static const u8 PNG_SIGNATURE[8] = {0x89, 0x50, 0x4e, 0x47,
                                      0x0d, 0x0a, 0x1a, 0x0a};
  if (memcmp(context->datastream, PNG_SIGNATURE, 8) != 0) {
    return false;
  }

  context->current_index += 8;
  return true;
}

u32 peek_next_chunk_length(ParsingContext *context) {
  return u32_from_bytes(context->datastream + context->current_index);
}

u32 parse_chunk_length(ParsingContext *context) {
  u32 length = u32_from_bytes(context->datastream + context->current_index);
  context->current_index += CHUNK_TYPE_SIZE;
  return length;
}

ChunkType chunk_type_from_str(const char *str) {
  if (strncmp(str, "IHDR", CHUNK_TYPE_SIZE) == 0) {
    return ChunkType_IHDR;
  } else if (strncmp(str, "IDAT", CHUNK_TYPE_SIZE) == 0) {
    return ChunkType_IDAT;
  } else if (strncmp(str, "IEND", CHUNK_TYPE_SIZE) == 0) {
    return ChunkType_IEND;
  }
  return ChunkType_Unknown;
}

// This function assumes the current parsing context is at the beginning of a
// new chunk
ChunkType peek_next_chunk_type(ParsingContext *context) {
  const char *type_str = (const char *)context->datastream +
                         context->current_index + CHUNK_LENGTH_SIZE;
  return chunk_type_from_str(type_str);
}

ChunkType parse_chunk_type(ParsingContext *context) {
  const char *datastream =
      (const char *)context->datastream + context->current_index;
  context->current_index += CHUNK_TYPE_SIZE;
  return chunk_type_from_str(datastream);
}

void skip_u32(ParsingContext *context) { context->current_index += 4; }

u32 parse_u32(ParsingContext *context) {
  const u32 value =
      u32_from_bytes(context->datastream + context->current_index);
  context->current_index += 4;
  return value;
}

u8 parse_u8(ParsingContext *context) {
  const u8 value = (u8)context->datastream[context->current_index];
  context->current_index++;
  return value;
}

ColourType parse_colour_type(ParsingContext *context) {
  u8 colour_type = parse_u8(context);
  switch (colour_type) {
  case 2:
    return ColourType_Truecolour;
  default:
    break;
  }

  return ColourType_Unknown;
}

bool parse_ihdr_chunk(ParsingContext *context, Image *image) {
  static const u32 IHDR_CHUNK_LENGTH = 13;

  LOG_PNG_DECODER("Parsing IHDR chunk...");
  u32 length = parse_chunk_length(context);
  if (length != IHDR_CHUNK_LENGTH) {
    LOG_PNG_DECODER_ERROR(
        "IHDR chunk has the wrong length, expected %u, actual length is %u",
        IHDR_CHUNK_LENGTH, length);
  }
  LOG_PNG_DECODER("Chunk length: %u", length);

  ChunkType type = parse_chunk_type(context);
  if (type != ChunkType_IHDR) {
    LOG_PNG_DECODER_ERROR("Invalid chunk type");
    return false;
  }

  u32 width = parse_u32(context);
  u32 height = parse_u32(context);
  u8 bit_depth = parse_u8(context);
  ColourType colour_type = parse_colour_type(context);
  u8 compression_method = parse_u8(context);
  u8 filter_method = parse_u8(context);
  u8 interlace_method = parse_u8(context);

  LOG_PNG_DECODER("\nwidth: %u\nheight %u\nbit_depth: %u\ncolour_type: %u",
                  width, height, bit_depth, colour_type);

  if (colour_type == ColourType_Unknown) {
    LOG_PNG_DECODER_ERROR("Unsupported colour type: %u", colour_type);
    return false;
  }

  if (compression_method != 0) {
    LOG_PNG_DECODER_ERROR("Unsupported compression method: %u",
                          compression_method);
    return false;
  }

  image->width = width;
  image->height = height;
  image->bit_depth = bit_depth;

  u32 crc = parse_u32(context);

  return true;
}

// This function assumes the current index is at the start of a chunk
void skip_chunk(ParsingContext *context) {
  u32 data_length = peek_next_chunk_length(context);
  context->current_index +=
      CHUNK_LENGTH_SIZE + CHUNK_TYPE_SIZE + data_length + CHUNK_CRC_SIZE;
}

// This function assumes the current index is at the start of a chunk
void skip_unsupported_chunks(ParsingContext *context) {
  // Assuming we are at a start of a chunk, we peek at the type
  ChunkType type = peek_next_chunk_type(context);
  while (type == ChunkType_Unknown) {
    skip_chunk(context);
    type = peek_next_chunk_type(context);
  }
}

void parse_idat_chunk(ParsingContext *context, u8vec *compressed_data) {
  u32 length = parse_chunk_length(context);
  ChunkType type = parse_chunk_type(context);
  ASSERT(type == ChunkType_IDAT);
  u8vec_append(compressed_data, context->datastream + context->current_index,
               length);
  context->current_index += length;
  // TODO validate crc
  skip_u32(context);
}

Image *png_load(const u8 *datastream) {
  ASSERT(datastream);

  Image *image = memory_allocate(sizeof(Image));
  if (!image) {
    goto err;
  }

  ParsingContext context = {.datastream = datastream, .current_index = 0};
  if (!parse_signature(&context)) {
    LOG_PNG_DECODER_ERROR("Invalid signature");
    goto cleanup_image;
  }

  if (!parse_ihdr_chunk(&context, image)) {
    LOG_PNG_DECODER_ERROR("Couldn't parse header");
    goto cleanup_image;
  }

  // NOTE: Bit depth might not be equal to the size of a pixel
  size_t image_data_size = image->width * image->height * image->bit_depth;
  image->data =
      memory_allocate_array(image->width * image->height, image->bit_depth);
  if (!image->data) {
    goto cleanup_image;
  }

  skip_unsupported_chunks(&context);

  ChunkType next_chunk_type = peek_next_chunk_type(&context);
  u8vec compressed_data;
  u8vec_init(&compressed_data);
  while (next_chunk_type == ChunkType_IDAT) {
    parse_idat_chunk(&context, &compressed_data);
    next_chunk_type = peek_next_chunk_type(&context);
  }

  read_zlib_compressed_data(compressed_data.data, image->data, image_data_size);

  for (size_t i = 0; i < image_data_size; i++) {
    LOG_DEBUG("%x", image->data[i]);
  }
  u8vec_deinit(&compressed_data);

  skip_unsupported_chunks(&context);
  next_chunk_type = peek_next_chunk_type(&context);
  if (next_chunk_type != ChunkType_IEND) {
    LOG_PNG_DECODER_ERROR("Doesn't end with IEND chunk");
    goto cleanup_image_data;
  }

  return image;

cleanup_image_data:
  memory_free(image->data);
cleanup_image:
  memory_free(image);
err:
  return NULL;
}
