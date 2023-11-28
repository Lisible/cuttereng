#ifndef CUTTERENG_MEMORY_H
#define CUTTERENG_MEMORY_H

#include "common.h"
/// @file

/// Allocates `size` bytes
///
/// Similar to `malloc()`
/// @return a pointer to the newly allocated data or NULL if an error occurs
void *memory_allocate(size_t size);

/// Allocate an array of `count` elements, each of size `size`
///
/// Sets the allocated memory to 0
/// Similar to `calloc()`
/// @return a pointer to the newly allocated data or NULL if an error occurs
void *memory_allocate_array(size_t count, size_t item_size);

/// Reallocate a memory block
///
/// Similar to `realloc()`
/// @return the pointer to the reallocated data
void *memory_reallocate(void *ptr, size_t new_size);

/// Frees the memory space pointed by `ptr`
///
/// Similar to `free()`
void memory_free(void *ptr);

#endif
