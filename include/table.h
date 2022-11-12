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
    struct column* last_column; //указатель на послежний элемнт списка колонок
};

struct table_header {
    char name[MAX_NAME_LEN];
    struct database* db;
    struct table* table;
    uint64_t row_count;

    uint32_t page_count; //количество страниц в таблице
    struct page* starting_page; //начало записей aka первая страница
    struct page* current_page; //место где остановились и свободно для записи  

    struct table_header* next; 

    uint32_t number_in_tech_page;
    bool valid;
};

struct table {
    struct table_header* table_header;
    struct table_schema* table_schema; //мб надо убрать указатель
};

struct resultset {
    uint64_t length;
    char* cursor; //указатель на начало данных
};

bool column_exists(const struct column* column_list, const size_t len, const char* name);
//size_t column_list_length(const struct column* column_list);
void destroy_column_list(struct column* column_list);
//struct column* column_list_last(struct column* column_list);
void add_back_column_to_list(struct table_schema* schema, const char* column_name, enum data_type column_type);
struct column* delete_column_from_list(struct column* cur, const char* column_name, struct table_schema* schema);
struct column* create_column(const char* column_name, enum data_type column_type );
struct table_schema* create_table_schema();
struct table_schema* add_column_to_schema(struct table_schema* schema, const char* column_name, enum data_type column_type);
struct table_schema* delete_column_from_schema(struct table_schema* schema, const char* column_name);


#endif
