#include <inttypes.h>
#include <stdlib.h>

#define MAX_TABLE_NAME_LEN 20
#define MAX_COLUMN_NAME_LEN 20
#define INITIAL_ARRAY_SIZE 1

struct query {
    enum query_type type;

    char* table_name[MAX_TABLE_NAME_LEN];
    char column_names[INITIAL_ARRAY_SIZE][MAX_COLUMN_NAME_LEN]; //TODO
    void* column_values[INITIAL_ARRAY_SIZE];
    
    uint64_t rows_number;
};

struct query_join {
    char* first_table_name[MAX_TABLE_NAME_LEN];
    char* second_table_name[MAX_TABLE_NAME_LEN];
    char first_column_names[INITIAL_ARRAY_SIZE][MAX_COLUMN_NAME_LEN]; //TODO
    char second_column_names[INITIAL_ARRAY_SIZE][MAX_COLUMN_NAME_LEN]; //TODO
};

enum query_type {
    SELECT_ALL = 0,
    SELECT_SOME,
    UPDATE_WHERE,
    DELETE_WHERE,
    SELECT_WHERE,
};
