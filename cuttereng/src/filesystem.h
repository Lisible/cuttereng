#ifndef CUTTERENG_FILESYSTEM_H
#define CUTTERENG_FILESYSTEM_H
/// @file

#include "memory.h"

typedef struct {
  size_t entry_count;
  char (*entries)[256];
} DirectoryListing;

DirectoryListing *
filesystem_list_files_in_directory(Allocator *allocator,
                                   const char *directory_path);
void filesystem_directory_listing_destroy(Allocator *allocator,
                                          DirectoryListing *directory_listing);

/// Reads an entire file to string
///
/// The user is responsible for freeing the returned string
/// @return The file content
char *filesystem_read_file_to_string(Allocator *allocator, const char *path,
                                     size_t *out_size);

#endif // CUTTERENG_FILESYSTEM_H
