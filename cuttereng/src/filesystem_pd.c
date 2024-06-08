#include "filesystem.h"
DirectoryListing *
filesystem_list_files_in_directory(Allocator *allocator,
                                   const char *directory_path) {}
void filesystem_directory_listing_destroy(Allocator *allocator,
                                          DirectoryListing *directory_listing) {
}
char *filesystem_read_file_to_string(Allocator *allocator, const char *path,
                                     size_t *out_size) {}
