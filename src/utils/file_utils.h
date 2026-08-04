#ifndef UTIL_FILE_UTILS_H
#define UTIL_FILE_UTILS_H

#include <stddef.h>

char * read_file_to_string(const char * filepath, char * buffer, size_t buffer_size);

#endif // UTIL_FILE_UTILS_H
