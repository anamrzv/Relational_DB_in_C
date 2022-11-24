#ifndef DB_H
#define DB_H

#include <inttypes.h>
#include <stdio.h>
#include "file.h"
#include "table.h"

#define MAX_DB_NAME_LEN 50
#define DEFAULT_PAGE_SIZE_B 4096
#define MAX_TABLE_NAME_LEN 20

struct table_schema;
struct table_header;
enum query_type;

enum database_type {
    EXISTING = 0,
    TO_BE_CREATED
};

struct page_header {
    uint16_t free_bytes;
    uint32_t page_number_general;
    bool dirty;

    uint32_t free_space_cursor; //offset курсор там где можно писать, изначально это начало страницы

    char table_name[MAX_TABLE_NAME_LEN];
    uint32_t table_number_in_tech_page;

    uint32_t next_page_number_general;
};

struct database_header {
    char name[MAX_DB_NAME_LEN];
    struct database* db;

    uint32_t table_count;
    uint32_t page_count;

    uint32_t page_size; //по умолчанию 4 байт

    uint32_t last_page_general_number;
};

struct database {
    struct database_header* database_header;
    FILE* database_file;
};

struct page_header* create_page(struct database_header* db_header, struct table_header* table_header);
struct page_header* add_tech_page(struct database_header* db_header);
struct page_header* add_page(struct table_header* table_header, struct database_header* db_header);

struct database* get_prepared_database(const char *const filename, const enum database_type type);
struct database* create_database_in_file(const char *const filename);
struct database* get_database_from_file(const char *const filename);

struct table* create_table_from_schema(struct table_schema* schema, const char* table_name, struct database* db);
void delete_table(const char* tablename, struct database* db);
bool enough_free_space(struct page_header* page_header, uint32_t desired_volume);
void close_database(struct database* db);

struct table* get_table(const char *const tablename, struct database* db);

struct query* create_query(enum query_type type, struct table* tables, char* column[], void* values[], int32_t row_count);
struct query_join* create_query_join(struct table* left_table, struct table* right_table, char* left_column, char* right_column);
void run_query(struct query* query);
void run_join_query(struct query_join* query);

#endif