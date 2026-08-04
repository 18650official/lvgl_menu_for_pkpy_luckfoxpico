#include "utils/file_utils.h"
#include <stdio.h>

char * read_file_to_string(const char * filepath, char * buffer, size_t buffer_size)
{
    FILE * fp = fopen(filepath, "r");
    if (!fp) {
        snprintf(buffer, buffer_size, "Error: Cannot open %s", filepath);
        return buffer;
    }

    size_t read_len = fread(buffer, 1, buffer_size - 1, fp);
    buffer[read_len] = '\0';
    fclose(fp);
    return buffer;
}
