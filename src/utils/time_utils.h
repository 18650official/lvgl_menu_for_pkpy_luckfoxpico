#ifndef UTIL_TIME_UTILS_H
#define UTIL_TIME_UTILS_H

#include <stdbool.h>
#include <stddef.h>

void util_format_current_time(char * buf, size_t buf_size, bool show_seconds, bool is_24_hour_format);
void util_get_local_time(int * hour, int * minute);
void util_set_system_time(int hour, int minute);

#endif // UTIL_TIME_UTILS_H
