#include "../assert.h"
#include "../log.h"
#include "environment.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *get_process_base_path(Allocator *allocator, size_t *buffer_size);
char *get_process_executable_path(Allocator *allocator, size_t *buffer_size);

char *env_get_configuration_file_path(Allocator *allocator) {
  static const char CONFIGURATION_FILE_NAME[] = "/project_configuration.json";
  static const size_t CONFIGURATION_FILE_NAME_LENGTH =
      sizeof(CONFIGURATION_FILE_NAME);
  size_t buffer_size;
  char *configuration_file_path =
      get_process_base_path(allocator, &buffer_size);
  size_t executable_directory_path_length = strlen(configuration_file_path);
  configuration_file_path = allocator_reallocate(
      allocator, configuration_file_path, buffer_size,
      executable_directory_path_length + CONFIGURATION_FILE_NAME_LENGTH + 2);
  strcat(configuration_file_path, CONFIGURATION_FILE_NAME);
  return configuration_file_path;
}

char *get_symlink_target(Allocator *allocator, size_t *out_buffer_size,
                         char *symlink_path) {
  ASSERT(symlink_path != NULL);
  size_t buffer_size = 512;
  ssize_t result;

  char *buffer = allocator_allocate(allocator, buffer_size);
  if (!buffer)
    goto err_alloc;

  while ((result = readlink(symlink_path, buffer, buffer_size)) >=
         buffer_size) {
    size_t old_buffer_size = buffer_size;
    buffer_size *= 2;

    buffer =
        allocator_reallocate(allocator, buffer, old_buffer_size, buffer_size);
    if (!buffer)
      goto err_alloc;
  }

  if (result < 0)
    goto err_readlink;

  *out_buffer_size = buffer_size;
  buffer[result] = '\0';
  return buffer;

err_alloc:
  LOG_ERROR("Memory allocation failed while trying to fetch the executable "
            "path:\n\t%s",
            strerror(errno));
  return NULL;

err_readlink:
  LOG_ERROR("readlink() failed when trying to fetch the executable path:\n\t%s",
            strerror(errno));
  return NULL;
}
char *get_process_executable_path(Allocator *allocator, size_t *buffer_size) {
  return get_symlink_target(allocator, buffer_size, "/proc/self/exe");
}
char *get_process_base_path(Allocator *allocator, size_t *buffer_size) {
  return get_symlink_target(allocator, buffer_size, "/proc/self/cwd");
}
