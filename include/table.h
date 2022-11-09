#ifndef TABLE_H
#define TABLE_H

#include <inttypes.h>
#include <stdlib.h>

#define MAX_NAME_LEN 20
#define MAX_DATATYPE_LEN 10
#define INITIAL_ARRAY_SIZE 1

typedef enum data_type data_type;

struct column {
    char name[MAX_NAME_LEN];
    data_type type;
    uint16_t size;
};

struct table_schema {
    struct column columns[INITIAL_ARRAY_SIZE];    
};

struct table_header {
    char name[MAX_NAME_LEN];
    struct database* db;
    uint16_t column_count;
    uint64_t row_count;
};

struct table {
    struct table_header* table_header;
    struct table_schema* table_schema; //мб надо убрать указатель
    struct page* starting_page; //начало записей aka первая страница
    struct page* current_page; //место где остановились и свободно для записи   
};

enum data_type {
    TYPE_INT32 = 0,
    TYPE_BOOL,
    TYPE_STRIN,
    TYPE_FLOAT
};

struct resultset {
    uint64_t length;
    void* cursor; //указатель на начало данных
};


#endif