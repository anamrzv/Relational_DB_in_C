#ifndef DB_H
#define DB_H

#include <inttypes.h>
#include <stdio.h>
#include "../include/file.h"
#include "../include/table.h"

#define MAX_DB_NAME_LEN 50
#define DEFAULT_PAGE_SIZE_B 4096
#define MAX_TABLE_NAME_LEN 20

struct table_schema;

enum database_type {
    EXISTING = 0,
    TO_BE_CREATED
};

struct page_header {
    struct table_header* table_header;
    struct database_header* database_header;
    uint16_t free_bytes;
    uint32_t page_number_table; //номер страницы в общем списке страниц
    uint32_t page_number_general;
    bool dirty;

    uint32_t free_space_cursor; //offset курсор там где можно писать, изначально это начало страницы
    struct page* next; //связный список - след. страница этой таблицы

    char table_name[MAX_TABLE_NAME_LEN];
    uint32_t next_page_number_general;
};


struct page {
    struct page_header* page_header;
};

struct database_header {
    char name[MAX_DB_NAME_LEN];
    struct database* db;

    uint32_t table_count;
    uint32_t page_count;

    uint32_t page_size; //по умолчанию 4 байт, проверить тип данных
    
    struct page* first_page; //служебная страница
    struct page* last_tech_page;
    
    struct table_header* next; //первый табличный заголовок
    struct table_header* last_table_header;
};

struct database {
    struct database_header* database_header;
    FILE* database_file;
};

struct page* create_page(struct database_header* db_header, struct table_header* table_header);
void destroy_table_page_list(struct page* page_list);
struct page* add_page_back_to_db_header_list(struct database_header* db_header);
struct page* add_page_back_to_table_header_list(struct table_header* table_header);

struct database* get_prepared_database(const char *const filename, const enum database_type type);
struct database* create_database_in_file(const char *const filename);
struct database* get_database_from_file(const char *const filename);

struct table* create_table_from_schema(struct table_schema* schema, const char* table_name, struct database* db);
void delete_table(const char* table_name, struct database* db);
bool enough_free_space(struct page* page, uint32_t desired_volume);
void close_database(struct database* db);

struct table* get_table(const char *const tablename, struct database* db);

#endif