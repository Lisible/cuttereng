#include "filesystem.h"
#include <dirent.h>
#include <errno.h>
#include <lisiblestd/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DirectoryListing *
filesystem_list_files_in_directory(Allocator *allocator,
                                   const char *directory_path) {
  DirectoryListing *result =
      Allocator_allocate(allocator, sizeof(DirectoryListing));
  if (!result)
    goto err;

  DIR *d;
  struct dirent *dir;
  d = opendir(directory_path);
  if (!d) {
    LOG_ERROR("opendir() failed: %s", strerror(errno));
    goto err_open_dir;
  }

  size_t file_count = 0;
  while ((dir = readdir(d)) != NULL) {
    if (dir->d_type == DT_REG) {
      file_count++;
    }
  }
  rewinddir(d);
  result->entry_count = file_count;
  result->entries =
      Allocator_allocate_array(allocator, file_count, sizeof(char[256]));
  if (!result->entries) {
    goto err_alloc_entries;
  }
  size_t i = 0;
  while ((dir = readdir(d)) != NULL) {
    if (dir->d_type == DT_REG) {
      file_count++;
      memcpy(result->entries[i], dir->d_name, 256);
      i++;
    }
  }
  closedir(d);

  return result;
err_alloc_entries:
  closedir(d);
err_open_dir:
  Allocator_free(allocator, result);
err:
  return NULL;
}
void filesystem_directory_listing_destroy(Allocator *allocator,
                                          DirectoryListing *directory_listing) {
  Allocator_free(allocator, directory_listing->entries);
  Allocator_free(allocator, directory_listing);
}

char *filesystem_read_file_to_string(Allocator *allocator, const char *path,
                                     size_t *out_size) {
  if (!path)
    goto err;

  FILE *file = fopen(path, "r");
  if (file == NULL)
    goto err;
  if (fseek(file, 0, SEEK_END) < 0)
    goto err_2;

  long length = ftell(file);

  if (length < 0)
    goto err_2;

  if (fseek(file, 0, SEEK_SET) < 0)
    goto err_2;

  char *content = Allocator_allocate(allocator, length + 1);
  if (content == NULL)
    goto err_2;

  if (fread(content, 1, length, file) != (unsigned long)length)
    goto err_3;

  content[length] = '\0';

  if (fclose(file) < 0)
    goto err_3;

  if (out_size) {
    *out_size = length;
  }

  return content;

err_3:
  free(content);

err_2:
  fclose(file);

err:
  LOG_ERROR("An error occured while reading file: %s", strerror(errno));
  return NULL;
}
