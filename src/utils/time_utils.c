#include "utils/time_utils.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

void util_format_current_time(char * buf, size_t buf_size, bool show_seconds, bool is_24_hour_format)
{
    time_t t = time(NULL);
    struct tm * tm_info = localtime(&t);
    const char * format_str = is_24_hour_format ? (show_seconds ? "%H:%M:%S" : "%H:%M")
                                                : (show_seconds ? "%I:%M:%S %p" : "%I:%M %p");
    strftime(buf, buf_size, format_str, tm_info);
}

void util_get_local_time(int * hour, int * minute)
{
    time_t t = time(NULL);
    struct tm * tm_info = localtime(&t);
    if (hour) {
        *hour = tm_info->tm_hour;
    }
    if (minute) {
        *minute = tm_info->tm_min;
    }
}

void util_set_system_time(int hour, int minute)
{
    char command[64];
    snprintf(command, sizeof(command), "date -s \"%02d:%02d:00\"", hour, minute);
    system(command);
    system("hwclock -w");
}
