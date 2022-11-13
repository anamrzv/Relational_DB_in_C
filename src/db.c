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
        db_header->page_count += 1;

        created_page_header->page_number_general = db_header->page_count;
        created_page_header->page_number_table = db_header->page_count;
        created_page_header->database_header = db_header;
        created_page_header->table_header = NULL;
    } else if (table_header != NULL) {
        table_header->page_count += 1;
        table_header->db->database_header->page_count += 1;

        created_page_header->page_number_general = table_header->db->database_header->page_count;
        created_page_header->page_number_table = table_header->page_count;
        created_page_header->table_header = table_header;
        created_page_header->database_header = NULL;
    } else return NULL;
    
    created_page->page_header = created_page_header;
    created_page->page_header->next = NULL;
    created_page->page_header->free_space_cursor = 0; //offset = 0

    return created_page;
}

void destroy_table_page_list(struct page* page_list) {
    struct page* next;
    struct page* cur=page_list;
    while (cur) {
        next = cur->page_header->next;
        free(cur);
        cur = next;
    }
}

bool table_exists(struct table_header* th_list, const size_t len, const char* name) {
    int32_t index = 0;
    const struct table_header* cur = th_list;
    if (th_list != NULL){
        while (index != len) {
            if (strcmp(cur->name, name) == 0) return true;
            index++;
            cur=cur->next;
        }
        return false;
    } 
    else return false;
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
            table_header->current_page->page_header->next = new_page;
        } else table_header->starting_page = new_page;
        table_header->current_page = new_page;
    }
}

struct database* get_prepared_database(const char *const filename, const enum database_type type) {
    switch (type) {
        case EXISTING:
            return get_database_from_file(filename);
        case TO_BE_CREATED:
            return create_database_in_file(filename);
    }
}

struct database* create_database_in_file(const char *const filename) {
    struct database* db = malloc(sizeof(struct database) + sizeof(struct database_header));

    FILE *in = NULL;
    switch (open_file(&in, filename, "w+"))
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
    db_header->last_table_header = NULL;
    db_header->db = db;
    add_page_back_to_db_header_list(db_header); //служебная страница

    db->database_file = in;
    db->database_header = db_header;
    
    write_header_to_tech_page(in, db_header->first_page, NULL);

    return db;
}

struct database* get_database_from_file(const char *const filename) {
    //TODO
}

struct table* create_table_from_schema(struct table_schema* schema, const char* table_name, struct database* db) {
    if (table_exists(db->database_header->next, db->database_header->table_count, table_name)) {
        printf("Таблица с таким именем существует. Нельзя создать.");
        return NULL;
    }
    struct table* created_table = malloc(sizeof(struct table));
    created_table->table_schema = schema;

    struct table_header* table_header = malloc(sizeof(struct table_header));
    strncpy(table_header->name, "", MAX_NAME_LEN);
    strncpy(table_header->name, table_name, strlen(table_name));
    table_header->db = db;
    table_header->row_count = 0;
    table_header->page_count = 0;
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

    if (db->database_header->last_table_header == NULL) db->database_header->last_table_header = table_header;
    else {
        db->database_header->last_table_header->next = table_header;
        db->database_header->last_table_header = table_header;
    }
    
    write_header_to_tech_page(db->database_file, db->database_header->first_page, table_header->current_page);
    write_table_page_first_time(db->database_file, table_header->current_page);

    return created_table;
}

void delete_table(const char* table_name, struct database* db) {
    size_t index = 0;
    struct database_header* left_to_cur_db = db->database_header;
    struct table_header* left_to_cur = NULL;

    struct table_header* cur = db->database_header->next;
    uint32_t table_count = db->database_header->table_count;
    if (table_count != 0) {
        while (index != table_count) {
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
        if (index == table_count) printf("Таблицы с таким названием нет");
    }
}


