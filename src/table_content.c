#include "../include/table.h"

struct row* create_row(struct table* table) {
    struct row* new_row = malloc(sizeof(struct row));
    struct row_header* row_header = malloc(sizeof(struct row_header));
    new_row->table = table;
    new_row->content = malloc(table->table_schema->row_length); //начало содержимого строки

    row_header->valid = true;
    new_row->row_header = row_header;

    return new_row;
}

//TODO проверка что имя колонки и тип правильные
void fill_with_int(struct row* row, int32_t value, uint32_t offset) {
    char* pointer_to_write = (char*) row->content + offset;
    *((int32_t*) (pointer_to_write)) = value;
}

void fill_with_bool(struct row* row, bool value, uint32_t offset) {
    char* pointer_to_write = (char*) row->content + offset;
    *((bool*) (pointer_to_write)) = value;
}

void fill_with_string(struct row* row, char* value, uint32_t offset, uint32_t string_len) {
    if (string_len != -1) {
        char* pointer_to_write = (char*) row->content + offset;
        strncpy(pointer_to_write, "", string_len);
        strncpy(pointer_to_write, value, strlen(value));
    }
}

void fill_with_float(struct row* row, double value, uint32_t offset) {
    char* pointer_to_write = (char*) row->content + offset;
    *((double*) (pointer_to_write)) = value;
}

int32_t column_offset(const struct column* column_list, const size_t len, const char* name) {
    int32_t index = 0;
    uint32_t offset = 0;
    if (column_list != NULL){
        while (index != len) {
            struct column col = column_list[index]; 
            uint32_t b = column_list[index].size;
            if (strcmp(column_list[index].name, name) == 0) return offset;
            index++;
            offset += b;
        }
        return -1;
    } 
    else return -1;
}

void fill_row_attribute(struct row* row, const char* column_name, enum data_type column_type, void* value) {
    read_columns_of_table(row->table->table_header->db->database_file, row->table); //нужно для таблиц созданных до
    uint32_t offset = column_offset(row->table->table_schema->columns, row->table->table_schema->column_count, column_name);
    if (offset != -1) {
        switch (column_type) {
            case TYPE_INT32:
                fill_with_int(row, *((int32_t*) value), offset);
                break;
            case TYPE_BOOL:
                fill_with_bool(row, *((bool*) value), offset);
                break;
            case TYPE_STRING:
                fill_with_string(row, *((char**) value), offset, string_column_length(row->table->table_schema->columns, row->table->table_schema->column_count, column_name));
                break;
            case TYPE_FLOAT:
                fill_with_float(row, *((double*) value), offset);
                break;
        }
    } else printf("Невозможно заполнить строку: Колонки с таким названием атибута не существует\n");   
}

void insert_row_to_table(struct row* row) {
    uint32_t page_with_free_space_num = row->table->table_header->last_page_general_number;
    enum write_status answer = write_row_to_page(row->table->table_header->db->database_file, page_with_free_space_num, row);
    if (answer != WRITE_OK) printf("Строка не записалась");
}

void select_row_from_table(struct query* query) {
    bool column_exists = false;
    enum data_type column_type;
    char column_name[MAX_COLUMN_NAME_LEN];
    uint16_t column_size = 0;
    
    for (size_t i=0; i<query->table->table_schema->column_count; i++) {
        if (strcmp(query->table->table_schema->columns[i].name, query->column_name[0]) == 0) {
            column_exists = true;
            column_type = query->table->table_schema->columns[i].column_type;
            strncpy(column_name, query->table->table_schema->columns[i].name, MAX_COLUMN_NAME_LEN);
            column_size = query->table->table_schema->columns[i].size;
            break;
        };
    }
    if (column_exists) {
        uint32_t offset = column_offset(query->table->table_schema->columns, query->table->table_schema->column_count, column_name[0]);
        select_where(query->table->table_header->db->database_file, query->table, offset, column_size, query->column_value[0], column_type, query->rows_number);
    } else printf("Невозможно выполнить запрос по вашему условию: колонки из запроса нет в таблице\n");
}

void update_row_in_table(struct query* query) {
    bool first_column_exists = false;
    bool second_column_exists = false;
    enum data_type first_column_type;
    enum data_type second_column_type;
    char first_column_name[MAX_COLUMN_NAME_LEN];
    char second_column_name[MAX_COLUMN_NAME_LEN];
    uint16_t first_column_size = 0;
    uint16_t second_column_size = 0;

    for (size_t i=0; i<query->table->table_schema->column_count; i++) {
        if (strcmp(query->table->table_schema->columns[i].name, query->column_name[0]) == 0) {
            first_column_exists = true;
            first_column_type = query->table->table_schema->columns[i].column_type;
            strncpy(first_column_name, query->table->table_schema->columns[i].name, MAX_COLUMN_NAME_LEN);
            first_column_size = query->table->table_schema->columns[i].size;
        } else if (strcmp(query->table->table_schema->columns[i].name, query->column_name[1]) == 0) {
            second_column_exists = true;
            second_column_type = query->table->table_schema->columns[i].column_type;
            strncpy(second_column_name, query->table->table_schema->columns[i].name, MAX_COLUMN_NAME_LEN);
            second_column_size = query->table->table_schema->columns[i].size;
        };
        if (first_column_exists && second_column_exists) break;
    }
    
    if (first_column_exists && second_column_exists) {
        uint32_t first_offset = column_offset(query->table->table_schema->columns, query->table->table_schema->column_count, first_column_name);
        uint32_t second_offset = column_offset(query->table->table_schema->columns, query->table->table_schema->column_count, second_column_name);
        struct expanded_query first_expanded = {first_column_type, first_column_name, first_column_size, first_offset};
        struct expanded_query second_expanded = {second_column_type, second_column_name, second_column_size, second_offset};
        update_where(query->table->table_header->db->database_file, query->table, &first_expanded, &second_expanded, query->column_value);
    } else printf("Невозможно выполнить запрос по вашему условию: колонки_jr из запроса нет в таблице\n");
}

void delete_row_from_table(struct query* query) {
    bool column_exists = false;
    enum data_type column_type;
    char column_name[MAX_COLUMN_NAME_LEN];
    uint16_t column_size = 0;
    
    for (size_t i=0; i<query->table->table_schema->column_count; i++) {
        if (strcmp(query->table->table_schema->columns[i].name, query->column_name) == 0) {
            column_exists = true;
            column_type = query->table->table_schema->columns[i].column_type;
            strncpy(column_name, query->table->table_schema->columns[i].name, MAX_COLUMN_NAME_LEN);
            column_size = query->table->table_schema->columns[i].size;
            break;
        };
    }
    if (column_exists) {
        uint32_t offset = column_offset(query->table->table_schema->columns, query->table->table_schema->column_count, column_name);
        delete_where(query->table->table_header->db->database_file, query->table, offset, column_size, query->column_value, column_type, query->rows_number);
    } else printf("Невозможно выполнить запрос по вашему условию: колонки из запроса нет в таблице\n");
}

