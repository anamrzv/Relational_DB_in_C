#include "../include/db.h"
#include "../include/file.h"

struct page* create_page(struct database_header* db_header, struct table_header* table_header) {
    struct page* created_page = malloc(sizeof (struct page) + sizeof (struct page_header));
    if (created_page == NULL) {
        return NULL;
    }
    struct page_header* created_page_header = malloc(sizeof (struct page_header));
    created_page_header->dirty = false;
    created_page_header->free_bytes = DEFAULT_PAGE_SIZE_B;
    
    if (db_header != NULL) {
        created_page_header->page_number = db_header->page_count;
        created_page_header->database_header = db_header;
        created_page_header->table_header = NULL;
    } else if (table_header != NULL) {
        created_page_header->page_number = table_header->page_count;
        created_page_header->table_header = table_header;
        created_page_header->database_header = NULL;
    } else return NULL;
    
    created_page->page_header = created_page_header;
    created_page->next = NULL;
    created_page->free_space_cursor = (char *) created_page;

    db_header->page_count += 1;

    return created_page;
}

void destroy_table_page_list(struct page* page_list) {
    struct column* next;
    struct column* cur=page_list;
    while (cur) {
        next = cur->next;
        free(cur);
        cur = next;
    }
}

void add_page_back_to_db_header_list(struct database_header* db_header) {
    struct page* new_page = create_page(db_header, NULL);
    if (new_page != NULL) {
        if (db_header->first_page == NULL) {
            db_header->first_page = new_page;
        }
    }
}

void add_page_back_to_table_header_list(struct table_header* table_header) {
    struct page* new_page = create_page(NULL, table_header);
    if (new_page != NULL) {
        if (table_header->current_page != NULL) {
            table_header->current_page->next = new_page;
        } else table_header->starting_page = new_page;
        table_header->current_page = new_page;
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
    db_header->page_count = 0;
    db_header->page_size = DEFAULT_PAGE_SIZE_B;
 
    db_header->first_page = NULL;
    db_header->next = NULL;
    add_page_back_to_db_header_list(db_header); //служебная страница

    db->database_file = in;
    db->database_header = db_header;
    
    write_header_to_tech_page(in, db_header->first_page, NULL);
}

struct database* get_database_from_file(const char *const filename) {
    //TODO
}

//TODO проверка что такого имени еще нет
struct table* create_table_from_schema(struct table_schema* schema, const char* table_name, struct database* db) {
    struct table* created_table = malloc(sizeof(struct table)+sizeof(struct table_header));
    created_table->table_schema = schema;

    struct table_header* table_header = malloc(sizeof(struct table_header));
    strncpy(table_header->name, "", MAX_NAME_LEN);
    strncpy(table_header->name, table_name, strlen(table_name));
    table_header->db = db;
    table_header->row_count = 0;
    table_header->page_count = 1;
    table_header->starting_page = NULL;
    table_header->current_page = NULL;
    table_header->next = NULL;
    table_header->valid = true;
    table_header->table = created_table;

    add_page_back_to_table_header_list(table_header);
    created_table->table_header = table_header;
    
    if (db->database_header->table_count == 0) db->database_header->next = table_header;
    db->database_header->table_count += 1;

    table_header->number_in_tech_page = db->database_header->table_count;

    write_header_to_tech_page(db->database_file, db->database_header->first_page, table_header->current_page);
}

void delete_table(const char* table_name, struct database* db) {
    size_t index = 0;
    struct database_header* left_to_cur_db = db->database_header;
    struct table_header* left_to_cur = NULL;

    struct table_header* cur = db->database_header->next;
    if (db->database_header->table_count != 0) {
        while (index != db->database_header->table_count) {
            if (strcmp(cur->name, table_name) == 0) {
                db->database_header->table_count -= 1;
                db->database_header->page_count -= cur->page_count;
                cur->valid = false;
                destroy_table_page_list(cur->starting_page); //удалили все страницы этой таблицы
                if (index == 0) overwrite_after_table_delete(db->database_file, cur, NULL, db->database_header);
                else overwrite_after_table_delete(db->database_file, cur, left_to_cur, NULL);
                free(cur->table);
                free(cur);
                break;
            } else {
                index++;
                left_to_cur = cur;
                cur = cur->next;
            }
        }
        printf("Таблицы с таким названием нет");
    }
}


