#include <inttypes.h>
#include "../include/file.h"

enum open_status open_file(FILE **in, const char *const filename, const char *const mode) {
    *in = fopen(filename, mode);
    if (*in == NULL)
    {
        return OPEN_ERROR;
    }
    else
        return OPEN_OK;
}

enum close_status close_file(FILE *in) {
    int64_t res = fclose(in);
    if (res != 0)
    {
        return CLOSE_ERROR;
    }
    else
        return CLOSE_OK;
}

enum write_status write_header_to_db_file(FILE *file, struct database_header* db_header) {
    fseek(file, 0, SEEK_SET);
    if (fwrite(db_header, sizeof(struct database_header), 1, file) == 1) return WRITE_OK;
    else return WRITE_ERROR;
}