#ifndef TABLE_H
#define TABLE_H

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <malloc.h>

#define MAX_NAME_LEN 20
#define MAX_DATATYPE_LEN 10
#define DEFAULT_STRING_LENGTH 255

enum data_type {
    TYPE_INT32 = 0,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_FLOAT
};

struct column {
    char name[MAX_NAME_LEN];
    enum data_type column_type;
    uint16_t size;
    struct column* next;
};

struct table_schema {
    uint64_t column_count;
    struct column* columns;  //указатель на начало св. списка
};

struct table_header {
    char name[MAX_NAME_LEN];
    struct database* db;
    uint64_t row_count;
};

struct table {
    struct table_header* table_header;
    struct table_schema* table_schema; //мб надо убрать указатель
    struct page* starting_page; //начало записей aka первая страница
    struct page* current_page; //место где остановились и свободно для записи   
};

struct resultset {
    uint64_t length;
    char* cursor; //указатель на начало данных
};

bool column_exists(const struct column* column_list, const size_t len, const char* name);
size_t column_list_length(const struct column* column_list);
void destroy_column_list(struct column* column_list);
struct column* column_list_last(struct column* column_list);
void add_back_column(struct column** old, const char* column_name, enum data_type column_type);
struct column* delete_column_from_list(struct column* cur, const char* column_name);
struct column* create_column(const char* column_name, enum data_type column_type );
struct table_schema* create_table_schema();
struct table_schema* add_column_to_schema(struct table_schema* schema, const char* column_name, enum data_type column_type);
struct table_schema* delete_column_from_schema(struct table_schema* schema, const char* column_name);


#endif
