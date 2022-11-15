#ifndef TABLE_H
#define TABLE_H

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <malloc.h>
#include "../include/file.h"

#define MAX_NAME_LEN 20
#define MAX_DATATYPE_LEN 10

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

    uint64_t row_length;
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

    struct table_schema schema;
    uint32_t first_page_general_number;
};

struct table {
    struct table_header* table_header;
    struct table_schema* table_schema; //мб надо убрать указатель
};

struct resultset {
    uint64_t length;
    char* cursor; //указатель на начало данных
};

struct row_header {
    bool valid;
};

struct row {
    struct row_header* row_header;
    struct table* table;
    void** content;
};

uint32_t column_exists(const struct column* column_list, const size_t len, const char* name);
void destroy_column_list(struct column* column_list);
void add_back_column_to_list(struct table_schema* schema, const char* column_name, enum data_type column_type);
void add_back_string_column_to_list(struct table_schema* schema, const char* column_name, enum data_type column_type, uint16_t size);
struct column* delete_column_from_list(struct column* cur, const char* column_name, struct table_schema* schema);
struct column* create_column(const char* column_name, enum data_type column_type);
struct column* create_string_column(const char* column_name, enum data_type column_type, uint16_t size);
struct table_schema* create_table_schema();
struct table_schema* add_column_to_schema(struct table_schema* schema, const char* column_name, enum data_type column_type);
struct table_schema* add_string_column_to_schema(struct table_schema* schema, const char* column_name, enum data_type column_type, uint16_t size);
struct table_schema* delete_column_from_schema(struct table_schema* schema, const char* column_name);

struct row* create_row(struct table* table);
void fill_with_int(struct row* row, int32_t value, uint32_t offset);
void fill_with_bool(struct row* row, bool value, uint32_t offset);
void fill_with_string(struct row* row, char* value, uint32_t offset, uint32_t string_len);
void fill_with_float(struct row* row, double value, uint32_t offset);
int32_t column_offset(const struct column* column_list, const size_t len, const char* name);
void fill_row_attribute(struct row* row, const char* column_name, enum data_type column_type, void* value);
void insert_row_to_table(struct row* row);
void delete_row_from_table(struct row* row);
void update_row_in_table(struct row* row);

#endif
