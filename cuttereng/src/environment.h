#ifndef CUTTERENG_ENVIRONMENT_H
#define CUTTERENG_ENVIRONMENT_H
/// @file

/// Get the configuration file path.
///
/// The returned pointer is owned by caller, and is responsible to free it.
/// @returns The process executable path
char *env_get_configuration_file_path();

#endif // CUTTERENG_ENVIRONMENT_H
