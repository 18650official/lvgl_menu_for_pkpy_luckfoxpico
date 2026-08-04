#ifndef UTIL_PREFS_H
#define UTIL_PREFS_H

#include <stdbool.h>

extern bool util_show_seconds;
extern bool util_is_24_hour_format;

void prefs_load(void);
void prefs_save(void);

#endif // UTIL_PREFS_H
