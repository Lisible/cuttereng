#ifndef CUTTERENG_FILESYSTEM_H
#define CUTTERENG_FILESYSTEM_H
/// @file

/// Reads an entire file to string
///
/// The user is responsible for freeing the returned string
/// @return The file content
char *filesystem_read_file_to_string(const char *path);

#endif // CUTTERENG_FILESYSTEM_H
