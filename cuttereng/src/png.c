#include "png.h"
#include "assert.h"
#include "bytes.h"
#include "common.h"
#include "log.h"
#include "memory.h"
#include "vec.h"
#include "zlib.h"
#include <math.h>
#include <string.h>

static const int CHUNK_LENGTH_SIZE = 4;
static const int CHUNK_TYPE_SIZE = 4;
static const int CHUNK_CRC_SIZE = 4;

typedef struct {
  Allocator *allocator;
  const u8 *datastream;
  size_t current_index;
} ParsingContext;

typedef enum {
  ChunkType_IHDR,
  ChunkType_IDAT,
  ChunkType_IEND,
  ChunkType_Unknown
} ChunkType;

typedef enum {
  ColourType_Truecolour = 2,
  ColourType_TruecolourWithAlpha = 4,
  ColourType_Unknown
} ColourType;

int colour_type_sample_count(ColourType colour_type) {
  switch (colour_type) {
  case ColourType_TruecolourWithAlpha:
    return 4;
  case ColourType_Truecolour:
  default:
    return 3;
  }
}

PixelFormat pixel_format_from_colour_type_and_bit_depth(ColourType colour_type,
                                                        u8 bit_depth) {
  if (bit_depth != 8) {
    return PixelFormat_Unknown;
  }

  if (colour_type == ColourType_TruecolourWithAlpha) {
    return PixelFormat_R8G8B8A8;
  }

  return PixelFormat_R8G8B8;
}

typedef struct {
  size_t length;
  ChunkType type;
  u8 *data;
  u32 crc;
} Chunk;

bool parse_signature(ParsingContext *context) {
  ASSERT(context != NULL);
  static const u8 PNG_SIGNATURE[8] = {0x89, 0x50, 0x4e, 0x47,
                                      0x0d, 0x0a, 0x1a, 0x0a};
  if (memcmp(context->datastream, PNG_SIGNATURE, 8) != 0) {
    return false;
  }

  context->current_index += 8;
  return true;
}

u32 peek_next_chunk_length(ParsingContext *context) {
  ASSERT(context != NULL);
  return u32_from_bytes(context->datastream + context->current_index);
}

u32 parse_chunk_length(ParsingContext *context) {
  ASSERT(context != NULL);
  u32 length = u32_from_bytes(context->datastream + context->current_index);
  context->current_index += CHUNK_TYPE_SIZE;
  return length;
}

ChunkType chunk_type_from_str(const char *str) {
  ASSERT(str != NULL);
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
  ASSERT(context != NULL);
  const char *type_str = (const char *)context->datastream +
                         context->current_index + CHUNK_LENGTH_SIZE;
  return chunk_type_from_str(type_str);
}

ChunkType parse_chunk_type(ParsingContext *context) {
  ASSERT(context != NULL);
  const char *datastream =
      (const char *)context->datastream + context->current_index;
  context->current_index += CHUNK_TYPE_SIZE;
  return chunk_type_from_str(datastream);
}

void skip_u32(ParsingContext *context) {
  ASSERT(context != NULL);
  context->current_index += 4;
}

u32 parse_u32(ParsingContext *context) {
  ASSERT(context != NULL);
  const u32 value =
      u32_from_bytes(context->datastream + context->current_index);
  context->current_index += 4;
  return value;
}

u8 parse_u8(ParsingContext *context) {
  ASSERT(context != NULL);
  const u8 value = (u8)context->datastream[context->current_index];
  context->current_index++;
  return value;
}

ColourType parse_colour_type(ParsingContext *context) {
  ASSERT(context != NULL);
  u8 colour_type = parse_u8(context);
  LOG_DEBUG("T %d", colour_type);
  switch (colour_type) {
  case 2:
    return ColourType_Truecolour;
  case 6:
    return ColourType_TruecolourWithAlpha;
  default:
    break;
  }

  return ColourType_Unknown;
}

bool parse_ihdr_chunk(ParsingContext *context, Image *image) {
  static const u32 IHDR_CHUNK_LENGTH = 13;
  ASSERT(context != NULL);
  ASSERT(image != NULL);

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
  if (filter_method != 0) {
    LOG_PNG_DECODER_ERROR("Unknown filter method: %d", filter_method);
    return false;
  }

  u8 interlace_method = parse_u8(context);
  if (interlace_method != 0) {
    LOG_PNG_DECODER_ERROR("Interlaced PNG (e.g. Adam7) are not supported");
    return false;
  }

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
  image->pixel_format =
      pixel_format_from_colour_type_and_bit_depth(colour_type, bit_depth);
  if (image->pixel_format == PixelFormat_Unknown) {
    LOG_PNG_DECODER_ERROR(
        "Unsupported pixel format, colour_type: %d, bit_depth: %d", colour_type,
        bit_depth);
    return false;
  }
  image->bytes_per_pixel =
      bit_depth / 8 * colour_type_sample_count(colour_type);

  // NOTE maybe compute chunk crc and check it (maybe behind
  // an option/build flag?)
  skip_u32(context);

  return true;
}

// This function assumes the current index is at the start of a chunk
void skip_chunk(ParsingContext *context) {
  ASSERT(context != NULL);
  u32 data_length = peek_next_chunk_length(context);
  context->current_index +=
      CHUNK_LENGTH_SIZE + CHUNK_TYPE_SIZE + data_length + CHUNK_CRC_SIZE;
}

// This function assumes the current index is at the start of a chunk
void skip_unsupported_chunks(ParsingContext *context) {
  ASSERT(context != NULL);
  // Assuming we are at a start of a chunk, we peek at the type
  ChunkType type = peek_next_chunk_type(context);
  while (type == ChunkType_Unknown) {
    skip_chunk(context);
    type = peek_next_chunk_type(context);
  }
}

void parse_idat_chunk(ParsingContext *context, u8vec *compressed_data) {
  ASSERT(context != NULL);
  ASSERT(compressed_data != NULL);
  u32 length = parse_chunk_length(context);
  ChunkType type = parse_chunk_type(context);
  ASSERT(type == ChunkType_IDAT);
  u8vec_append(compressed_data, context->datastream + context->current_index,
               length);
  context->current_index += length;
  // NOTE: maybe validate crc
  skip_u32(context);
}

u8 none_reconstruction_function(u8 recon_a, u8 recon_b, u8 recon_c, u8 x) {
  (void)recon_a;
  (void)recon_b;
  (void)recon_c;
  return x;
}
u8 sub_reconstruction_function(u8 recon_a, u8 recon_b, u8 recon_c, u8 x) {
  (void)recon_b;
  (void)recon_c;
  return x + recon_a;
}
u8 up_reconstruction_function(u8 recon_a, u8 recon_b, u8 recon_c, u8 x) {
  (void)recon_a;
  (void)recon_c;
  return x + recon_b;
}
u8 average_reconstruction_function(u8 recon_a, u8 recon_b, u8 recon_c, u8 x) {
  (void)recon_c;
  return x + floor((recon_a + recon_b) / 2.f);
}
int paeth_predictor(int a, int b, int c) {
  int p = a + b - c;
  int pa = abs(p - a);
  int pb = abs(p - b);
  int pc = abs(p - c);
  int pr;
  if (pa <= pb && pa <= pc) {
    pr = a;
  } else if (pb <= pc) {
    pr = b;
  } else {
    pr = c;
  }

  return pr;
}
u8 paeth_reconstruction_function(u8 recon_a, u8 recon_b, u8 recon_c, u8 x) {
  return x + paeth_predictor(recon_a, recon_b, recon_c);
}
typedef u8 (*ReconstructionFn)(u8, u8, u8, u8);
static const ReconstructionFn reconstruction_functions[] = {
    none_reconstruction_function, sub_reconstruction_function,
    up_reconstruction_function, average_reconstruction_function,
    paeth_reconstruction_function};

void apply_reconstruction_functions(Image *image,
                                    const u8 *decompressed_data_buffer) {
  ASSERT(image != NULL);
  ASSERT(decompressed_data_buffer != NULL);
  size_t bytes_per_pixel = image->bytes_per_pixel;
  for (u32 scanline = 0; scanline < image->height; scanline++) {
    int scanline_start_offset = scanline * (1 + image->width * bytes_per_pixel);
    u8 filter_type = decompressed_data_buffer[scanline_start_offset];
    int scanline_data_offset = scanline_start_offset + 1;
    for (u32 byte = 0; byte < image->width * bytes_per_pixel; byte++) {
      int a = 0;
      if (byte >= bytes_per_pixel) {
        a = image->data[scanline * image->width * bytes_per_pixel + byte -
                        bytes_per_pixel];
      }
      int b = 0;
      if (scanline > 0) {
        b = image->data[(scanline - 1) * image->width * bytes_per_pixel + byte];
      }
      int c = 0;
      if (scanline > 0 && byte >= bytes_per_pixel) {
        c = image->data[(scanline - 1) * image->width * bytes_per_pixel + byte -
                        bytes_per_pixel];
      }
      int x = decompressed_data_buffer[scanline_data_offset + byte];
      image->data[scanline * image->width * bytes_per_pixel + byte] =
          reconstruction_functions[filter_type](a, b, c, x);
    }
  }
}

Image *png_load(Allocator *allocator, const u8 *datastream) {
  ASSERT(datastream != NULL);

  Image *image = allocator_allocate(allocator, sizeof(Image));
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

  image->data = allocator_allocate_array(
      allocator, image->width * image->height, image->bytes_per_pixel);
  if (!image->data) {
    goto cleanup_image;
  }

  skip_unsupported_chunks(&context);

  ChunkType next_chunk_type = peek_next_chunk_type(&context);
  u8vec compressed_data;
  u8vec_init(allocator, &compressed_data);
  while (next_chunk_type == ChunkType_IDAT) {
    parse_idat_chunk(&context, &compressed_data);
    next_chunk_type = peek_next_chunk_type(&context);
  }

  u8vec decompressed_data_buffer;
  u8vec_init(allocator, &decompressed_data_buffer);
  if (read_zlib_compressed_data(compressed_data.data,
                                &decompressed_data_buffer) !=
      ZlibResult_Success) {
    LOG_PNG_DECODER_ERROR("Couldn't read zlib compressed data");
    goto cleanup_image_data;
  }

  apply_reconstruction_functions(image, decompressed_data_buffer.data);

  u8vec_deinit(&decompressed_data_buffer);
  u8vec_deinit(&compressed_data);
  skip_unsupported_chunks(&context);
  next_chunk_type = peek_next_chunk_type(&context);
  if (next_chunk_type != ChunkType_IEND) {
    LOG_PNG_DECODER_ERROR("Doesn't end with IEND chunk");
    goto cleanup_image_data;
  }

  return image;

cleanup_image_data:
  allocator_free(allocator, image->data);
cleanup_image:
  allocator_free(allocator, image);
err:
  return NULL;
}
