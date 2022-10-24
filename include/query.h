#include  <inttypes.h>
#include <stdlib.h>

#define MAX_NAME_LEN 20
#define INITIAL_ARRAY_SIZE 1

struct query {
    char name[MAX_NAME_LEN];
    char table_names[INITIAL_ARRAY_SIZE][MAX_NAME_LEN];
    char column_names[INITIAL_ARRAY_SIZE][MAX_NAME_LEN];
    char* col_values_pointers[INITIAL_ARRAY_SIZE];
    
    uint64_t rows_number;
};

enum query_type {
    SELECT_ALL = 0,
    UPDATE,
    DELETE,
    SELECT_WHERE
};
