#include  <inttypes.h>
#include <stdlib.h>

#define MAX_NAME_LEN 20
#define MAX_DATATYPE_LEN 10
#define INITIAL_ARRAY_SIZE 1

struct column {
    char name[MAX_NAME_LEN];
    char data_type[MAX_DATATYPE_LEN];
};

struct table_schema {
    struct column* columns[INITIAL_ARRAY_SIZE];
};

struct table {
    struct table_schema* table_schema;
    char name[MAX_NAME_LEN];
    char* rows_start; //начало записей
    char* table_start; //начало таблицы в файле
    uint64_t rows_count;
};

struct resultset {
    uint64_t length;
    char* cursor; //указатель на начало данных
};

enum data_type {
    TYPE_INT32 = 0,
    TYPE_BOOL,
    TYPE_STRIN,
    TYPE_FLOAT
};
