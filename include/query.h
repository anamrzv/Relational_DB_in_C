#include <inttypes.h>
#include <stdlib.h>
#include "table.h"

#define MAX_TABLE_NAME_LEN 20
#define MAX_COLUMN_NAME_LEN 20
#define INITIAL_ARRAY_SIZE 1

enum query_type {
    SELECT = 0,
    SELECT_WHERE,
    UPDATE_WHERE,
    DELETE_WHERE,
};

struct query {
    enum query_type type;

    struct table* table;
    char** column_name;
    void** column_value;

    int32_t rows_number;
};

struct query_join {
    struct table* first_table;
    struct table* second_table;
    char* first_column_names;
    char* second_column_names;
};

struct expanded_query {
    enum data_type column_type;
    char column_name[MAX_COLUMN_NAME_LEN];
    uint16_t column_size;
    uint32_t offset;
};
