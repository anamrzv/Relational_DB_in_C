#include "../include/db.h"
#include "../include/file.h"

struct page* create_page(struct database_header* db_header, struct table_header* table_header) {
     struct page_header* created_page_header = malloc(sizeof (struct page_header));
    if (created_page_header == NULL) {
        return NULL;
    }

    created_page_header->dirty = false;
    created_page_header->free_bytes = DEFAULT_PAGE_SIZE_B;
    created_page_header->free_space_cursor = 0;
    created_page_header->next_page_number_general = 0;
    created_page_header->page_number_general = db_header->page_count;

    if (table_header == NULL) {
        strncpy(created_page_header->table_name, "", MAX_TABLE_NAME_LEN);
    } else {
        strncpy(created_page_header->table_name, "", MAX_TABLE_NAME_LEN);
        strncpy(created_page_header->table_name, table_header->name, MAX_TABLE_NAME_LEN);
    }
    
    return created_page_header;
}

struct page_header* add_tech_page(struct database_header* db_header) {
    db_header->page_count += 1;
    struct page_header* new_page_header = create_page(db_header, NULL);
    if (new_page_header != NULL) {
        if (db_header->last_page_general_number != 0) {
            //TODO overwrite page header with new "next_page_number_general"
        }
        db_header->last_page_general_number = new_page_header->page_number_general;
        return new_page_header;
    } else return NULL;
}

struct page* add_page(struct table_header* table_header, struct database_header* db_header) {
    db_header->page_count += 1;
    table_header->page_count += 1;
    struct page_header* new_page_header = create_page(db_header, table_header);
    if (new_page_header != NULL) {
        if (table_header->last_page_general_number != 0) {
            //TODO overwrite page header with new "next_page_number_general"
        } else {
            table_header->first_page_general_number = new_page_header->page_number_general;
        }
        table_header->last_page_general_number = new_page_header->page_number_general;
        return new_page_header;
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
 
    db_header->last_page_general_number = 0;
    db_header->db = db;
    struct page_header* tech_page_header = add_tech_page(db_header);

    db->database_file = in;
    db->database_header = db_header;
    
    write_db_to_file(in, db_header, tech_page_header);

    return db;
}

struct database* get_database_from_file(const char *const filename) {
    struct database* db = malloc(sizeof(struct database));
    struct database_header* db_header = malloc(sizeof(struct database_header));

    FILE *in = NULL;
    switch (open_file(&in, filename, "r+"))
    {
    case OPEN_ERROR:
        printf("Ошибка при открытии файла\n");
        return NULL;
    case OPEN_OK:
        printf("Файл успешно открыт\n");
        break;
    }

    enum read_status read_status = read_database_header(in, db_header);
    if (read_status == READ_OK) {
        db->database_header = db_header;
        db->database_file = in;
        return db;
    } else {
        printf("Не удалось прочитать заголовок БД");
        return NULL;
    }

}

struct table* create_table_from_schema(struct table_schema* schema, const char* table_name, struct database* db) {
    if (table_exists(db->database_file, db->database_header->table_count, table_name)) {
        printf("Таблица с таким именем существует. Нельзя создать.");
        return NULL;
    }
    struct table* created_table = malloc(sizeof(struct table));
    created_table->table_schema = schema;

    struct table_header* table_header = malloc(sizeof(struct table_header));
    strncpy(table_header->name, "", MAX_NAME_LEN);
    strncpy(table_header->name, table_name, strlen(table_name));
    table_header->db = db;
    table_header->page_count = 0;
    table_header->valid = true;
    table_header->table = created_table;
    table_header->schema = *schema;
    table_header->first_page_general_number = 0;
    table_header->last_page_general_number = 0;

    struct page_header* new_page_header = add_page(table_header, db->database_header);
    created_table->table_header = table_header;
    
    db->database_header->table_count += 1;
    table_header->number_in_tech_page = db->database_header->table_count;
    
    write_header_to_tech_page(db->database_file, db->database_header, new_page_header);
    overwrite_dh_after_change(db->database_file, db->database_header);
    write_table_page_first_time(db->database_file, new_page_header);

    return created_table;
}

void delete_table(const char* tablename, struct database* db) {
    struct table_header* th_to_delete = malloc(sizeof(struct table_header));

    if (read_table_header(db->database_file, tablename, th_to_delete, db->database_header->table_count) == READ_OK) {
        th_to_delete->valid = false;
        db->database_header->table_count -= 1;
        db->database_header->page_count -= th_to_delete->page_count;

        overwrite_th_after_change(db->database_file, th_to_delete);
        overwrite_dh_after_change(db->database_file, db->database_header);

        free(th_to_delete);
    } else printf("Не удалось удалить таблицу с таким именем\n");
}

bool enough_free_space(struct page_header* page_header, uint32_t desired_volume) {
    if (page_header->free_bytes >= desired_volume) return true;
    else return false;
}

void close_database(struct database* db) {
    close_file(db->database_file);
}

struct table* get_table(const char* tablename, struct database* db) {
    struct table* new_table = malloc(sizeof(struct table));
    struct table_header* new_th = malloc(sizeof(struct table_header));

    if (read_table_header(db->database_file, tablename, new_th, db->database_header->table_count) == READ_OK) {
        new_table->table_header = new_th;
        new_table->table_schema = NULL;
        new_table->table_header->db = db;
        new_table->table_header->db = NULL;
        return new_table;
    } else {
        printf("Не удалось прочитать таблицу\n");
        return NULL;
    }
}

