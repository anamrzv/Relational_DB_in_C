#ifndef QUERY_H
#define QUERY_H

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
    enum query_type q_type;

    struct table* table;
    char** column_name;
    void** column_value;

    int32_t rows_number;
};

struct query_join {
    struct table* left_table;
    struct table* right_table;
    char* left_column_name;
    char* right_column_name;
};

#endif