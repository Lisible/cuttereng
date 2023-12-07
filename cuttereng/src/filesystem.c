#include "filesystem.h"
#include "src/log.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *read_file_to_string(const char *path) {
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

  char *content = malloc(length + 1);
  if (content == NULL)
    goto err_2;

  if (fread(content, 1, length, file) != length)
    goto err_3;

  content[length] = '\0';

  if (fclose(file) < 0)
    goto err_3;

  return content;

err_3:
  free(content);

err_2:
  fclose(file);

err:
  LOG_ERROR("An error occured while reading file: %s", strerror(errno));
  return NULL;
}
