#include "../include/db.h"
#include "../include/file.h"
#include "../include/query.h"

struct page_header* create_page(struct database_header* db_header, struct table_header* table_header) {
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
        created_page_header->table_number_in_tech_page = table_header->number_in_tech_page;
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
            overwrite_previous_last_page_db(db_header->db->database_file, db_header, new_page_header->page_number_general);
        }
        db_header->last_page_general_number = new_page_header->page_number_general;
        if (db_header->page_count!=1) overwrite_dh_after_change(db_header->db->database_file, db_header);
        return new_page_header;
    } else return NULL;
}

struct page_header* add_page(struct table_header* table_header, struct database_header* db_header) {
    db_header->page_count += 1;
    table_header->page_count += 1;
    struct page_header* new_page_header = create_page(db_header, table_header);
    if (new_page_header != NULL) {
        if (table_header->last_page_general_number != 0) {
            overwrite_previous_last_page(table_header->db->database_file, table_header->last_page_general_number, new_page_header->page_number_general);
        } else {
            table_header->first_page_general_number = new_page_header->page_number_general;
        }
        table_header->last_page_general_number = new_page_header->page_number_general;
        if (table_header->page_count != 1) {
            overwrite_th_after_change(db_header->db->database_file, table_header);
            overwrite_dh_after_change(db_header->db->database_file, db_header);
        }
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
    switch (open_file(&in, filename, "wb+"))
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
    switch (open_file(&in, filename, "rb+"))
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
        db_header->db = db;
        db->database_header = db_header;
        db->database_file = in;
        return db;
    } else {
        printf("Не удалось прочитать заголовок БД");
        return NULL;
    }

}

struct table* create_table_from_schema(struct table_schema* schema, const char* table_name, struct database* db) {
    struct table_header* th = malloc(sizeof(struct table_header));
    if (table_exists(db->database_file, db->database_header->table_count, table_name, th)) {
        printf("Таблица с таким именем существует. Нельзя создать.");
        free(th);
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
    
    write_header_to_tech_page(db->database_file, db->database_header, table_header);
    overwrite_dh_after_change(db->database_file, db->database_header);
    write_table_page(db->database_file, new_page_header, created_table->table_schema);

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
    struct table_schema* new_ts = malloc(sizeof(struct table_schema));

    if (read_table_header(db->database_file, tablename, new_th, db->database_header->table_count) == READ_OK) {
        new_table->table_header = new_th;
        new_table->table_schema = new_ts;
        read_columns_of_table(db->database_file, new_table);
        new_table->table_header->db = db;
        return new_table;
    } else {
        printf("Не удалось прочитать таблицу\n");
        return NULL;
    }
}

struct query* create_query(enum query_type q_type, struct table* tables, char* column[], void* values[], int32_t row_count) {
    struct query* new_query= malloc(sizeof(struct query));
    new_query->q_type = q_type;
    new_query->table = tables;
    new_query->column_name = column;  
    new_query->column_value = values;
    new_query->rows_number = row_count;

    return new_query;
}

struct query_join* create_query_join(struct table* left_table, struct table* right_table, char* left_column, char* right_column) {
    struct query_join* join_query = malloc(sizeof(struct query_join));
    join_query->left_table = left_table;
    join_query->right_table = right_table;
    join_query->left_column_name = left_column;
    join_query->right_column_name = right_column;

    return join_query;
}

void run_query(struct query* query) {
    switch (query->q_type) {
            case SELECT_WHERE:
                select_row_from_table(query);
                break;
            case UPDATE_WHERE:
                update_row_in_table(query);
                break;
            case DELETE_WHERE:
                delete_row_from_table(query);
                break;            
    }
}

void run_join_query(struct query_join* query) {
    bool first_column_exists = false;
    bool second_column_exists = false;
    enum data_type first_column_type;
    enum data_type second_column_type;
    char first_column_name[MAX_COLUMN_NAME_LEN];
    char second_column_name[MAX_COLUMN_NAME_LEN];
    uint16_t first_column_size = 0;
    uint16_t second_column_size = 0;

    for (size_t i=0; i<query->left_table->table_schema->column_count; i++) {
        if (strcmp(query->left_table->table_schema->columns[i].name, query->left_column_name) == 0) {
            first_column_exists = true;
            first_column_type = query->left_table->table_schema->columns[i].column_type;
            strncpy(first_column_name, query->left_table->table_schema->columns[i].name, MAX_COLUMN_NAME_LEN);
            first_column_size = query->left_table->table_schema->columns[i].size;
        }
        if (first_column_exists) break;
    }

    for (size_t i=0; i<query->right_table->table_schema->column_count; i++) {
        if (strcmp(query->right_table->table_schema->columns[i].name, query->right_column_name) == 0) {
            second_column_exists = true;
            second_column_type = query->right_table->table_schema->columns[i].column_type;
            strncpy(second_column_name, query->right_table->table_schema->columns[i].name, MAX_COLUMN_NAME_LEN);
            second_column_size = query->right_table->table_schema->columns[i].size;
        }
        if (second_column_exists) break;
    }

    if (first_column_exists && second_column_exists) {
        uint32_t first_offset = column_offset(query->left_table->table_schema->columns, query->left_table->table_schema->column_count, first_column_name);
        uint32_t second_offset = column_offset(query->right_table->table_schema->columns, query->right_table->table_schema->column_count, second_column_name);
        struct expanded_query* first_expanded = malloc(sizeof(struct expanded_query));
        struct expanded_query* second_expanded = malloc(sizeof(struct expanded_query));

        first_expanded->column_type = first_column_type;
        first_expanded->column_size = first_column_size;
        first_expanded->offset = first_offset;
        strncpy(first_expanded->column_name, "", MAX_COLUMN_NAME_LEN);
        strncpy(first_expanded->column_name, first_column_name, MAX_COLUMN_NAME_LEN);

        second_expanded->column_type = second_column_type;
        second_expanded->column_size = second_column_size;
        second_expanded->offset = second_offset;
        strncpy(second_expanded->column_name, "", MAX_COLUMN_NAME_LEN);
        strncpy(second_expanded->column_name, second_column_name, MAX_COLUMN_NAME_LEN);

        join(query->left_table->table_header->db->database_file, query->left_table, query->right_table, first_expanded, second_expanded);
    } else printf("Невозможно выполнить запрос по вашему условию: колонки_ок из запроса нет в таблице\n");
}