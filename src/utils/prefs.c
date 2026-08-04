#include "lvgl/lvgl.h"
#include "utils/prefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PREFS_FILE "/etc/menu_prefs.conf"

bool util_show_seconds = true;
bool util_is_24_hour_format = true;

void prefs_load(void)
{
    FILE * fp = fopen(PREFS_FILE, "r");
    if (!fp) {
        prefs_save();
        return;
    }

    char line[100];
    while (fgets(line, sizeof(line), fp)) {
        char key[50];
        int value;
        if (sscanf(line, "%[^=]=%d", key, &value) == 2) {
            if (strcmp(key, "SHOW_SECONDS") == 0) {
                util_show_seconds = (value == 1);
            } else if (strcmp(key, "IS_24_HOUR") == 0) {
                util_is_24_hour_format = (value == 1);
            }
        }
    }
    fclose(fp);
}

void prefs_save(void)
{
    FILE * fp = fopen(PREFS_FILE, "w");
    if (!fp) {
        LV_LOG_ERROR("Failed to open preferences file for writing.");
        return;
    }

    fprintf(fp, "SHOW_SECONDS=%d\n", util_show_seconds ? 1 : 0);
    fprintf(fp, "IS_24_HOUR=%d\n", util_is_24_hour_format ? 1 : 0);
    fclose(fp);
}
