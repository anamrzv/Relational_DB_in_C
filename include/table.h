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

//в свою очередь у контента есть ключ по которому 
struct row_content {
    char* key;
    char* content_pointer; //указателей на опр. ячейку
};

//у каждой строки должен быть хэш по которому достаем контент
struct row {
    char* key;
    struct row_content* content_start;
};

struct table {
    struct table_schema* table_schema;
    char name[MAX_NAME_LEN];
    struct row* rows[INITIAL_ARRAY_SIZE]; //массив записей
    uint64_t rows_count;
};

struct resultset {
    //...
};

enum data_type {
    TYPE_INT32 = 0,
    TYPE_BOOL,
    TYPE_STRIN,
    TYPE_FLOAT
};
