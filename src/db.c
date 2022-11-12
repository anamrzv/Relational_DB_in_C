#include "../include/db.h"

struct page* create_page(struct database_header* db_header, struct table_header* table_header) {
    struct page* created_page = malloc(sizeof (struct page) + sizeof (struct page_header));
    if (created_page == NULL) {
        return NULL;
    }
    struct page_header* created_page_header = malloc(sizeof (struct page_header));
    created_page_header->dirty = false;
    created_page_header->free_bytes = DEFAULT_PAGE_SIZE_KB;
    
    if (db_header != NULL) {
        created_page_header->page_number = db_header->page_count;
        created_page_header->database_header = db_header;
        created_page_header->table_header = NULL;
    } else if (table_header != NULL) {
        created_page_header->page_number = table_header->page_count;
        created_page_header->table_header = table_header;
        created_page_header->database_header = NULL;
    } else return NULL;
    
    created_page->page_header = *created_page_header;
    created_page->next = NULL;
    created_page->free_space_cursor = (char *) created_page;

    return created_page;
}

void add_page_back_to_db_header_list(struct database_header* db_header) {
    struct page* new_page = create_page(db_header, NULL);
    if (new_page != NULL) {
        if (db_header->current_page != NULL) {
        db_header->current_page->next = new_page;
        } else db_header->first_page = new_page;
        db_header->current_page = new_page;
    }
}

struct database* get_prepared_database(const char *const filename, const enum database_type type) {
    switch (type) {
        case EXISTING:
            get_database_from_file(filename);
            break;
        case TO_BE_CREATED:
            create_database_in_file(filename);
            break;
    }
}

struct database* create_database_in_file(const char *const filename) {
    struct database* db = malloc(sizeof(struct database) + sizeof(struct database_header));

    FILE *in = NULL;
    switch (open_file(&in, filename, "a+"))
    {
    case OPEN_ERROR:
        printf("Ошибка при создании файла");
        return NULL;
    case OPEN_OK:
        printf("\nФайл создался\n");
        break;
    }

    struct database_header* db_header = malloc(sizeof(struct database_header));
    strncpy(db_header->name, "", MAX_DB_NAME_LEN);
    strncpy(db_header->name, filename, strlen(filename));
    db_header->table_count = 0;
    db_header->page_count = 1;
    db_header->page_size = DEFAULT_PAGE_SIZE_KB;
 
    db_header->current_page = NULL;
    db_header->first_page = NULL;
    db_header->cursor = NULL; //TODO
    add_page_back_to_db_header_list(db_header); //первая страница в БД - служебная

    db->database_file = in;
    db->database_header = *db_header;
    
    write_header_to_db_file(in, db_header);
}

struct database* get_database_from_file(const char *const filename) {
    //TODO
}


