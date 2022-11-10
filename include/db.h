#ifndef DB_H
#define DB_H

#include <inttypes.h>
#include <stdio.h>

#define MAX_NAME_LEN 50

struct page_header {
    struct table_header* table_header;
    uint16_t free_bytes;
    uint32_t page_number;
    bool dirty;
};

//таблица это массив страниц
struct page {
    struct page_header* page_header;
    char* start_free_space;
    char* end_free_space;
    struct page* next_page; //связный список
};

struct database_header {
    char name[MAX_NAME_LEN];
    uint32_t table_count;
    uint32_t page_count;

    uint32_t page_size; //по умолчанию 8 байт, проверить тип данных
    struct page* current_page; //свободная страница или где стоит курсор
};

struct database {
    struct database_header database_header;
    FILE* database_file;
};

#endif