#ifndef CUTTERENG_ENVIRONMENT_ENVIRONMENT_H
#define CUTTERENG_ENVIRONMENT_ENVIRONMENT_H
/// @file

#include <lisiblestd/memory.h>

/// Get the configuration file path.
///
/// The returned pointer is owned by caller, and is responsible to free it.
/// @returns The process executable path
char *env_get_configuration_file_path(Allocator *);
/// Get the cwd path.
///
/// The returned pointer is owned by caller, and is responsible to free it.
/// @returns The process executable path
char *env_get_cwd_path(Allocator *);

#endif // CUTTERENG_ENVIRONMENT_ENVIRONMENT_H
