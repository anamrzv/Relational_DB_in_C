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
    //memcpy(row->content+offset, value, sizeof(int32_t));
}

void fill_with_bool(struct row* row, bool value, uint32_t offset) {
    char* pointer_to_write = (char*) row->content + offset;
    *((bool*) (pointer_to_write)) = value;
    //memcpy(row->content+offset, value, sizeof(bool));
}

void fill_with_string(struct row* row, char* value, uint32_t offset, uint32_t string_len) {
    char* pointer_to_write = (char*) row->content + offset;
    strncpy(pointer_to_write, "", string_len);
    strncpy(pointer_to_write, value, string_len);
    //memcpy(row->content+offset, value, string_len);
}

void fill_with_float(struct row* row, double value, uint32_t offset) {
    char* pointer_to_write = (char*) row->content + offset;
    *((double*) (pointer_to_write)) = value;
    //memcpy(row->content+offset, value, sizeof(double));
}

int32_t column_offset(const struct column* column_list, const size_t len, const char* name) {
    int32_t index = 0;
    uint32_t offset = 0;
    const struct column* cur = column_list;
    if (column_list != NULL){
        while (index != len) {
            if (strcmp(cur->name, name) == 0) return offset;
            index++;
            offset += cur->size;
            cur=cur->next;
        }
        return -1;
    } 
    else return -1;
}

void fill_row_attribute(struct row* row, const char* column_name, enum data_type column_type, void* value) {
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
                fill_with_string(row, *((char**) value), offset, column_exists(row->table->table_schema->columns, row->table->table_schema->column_count, column_name));
                break;
            case TYPE_FLOAT:
                fill_with_float(row, *((double*) value), offset);
                break;
        }
    } else printf("Колонки с таким названием не существует");   
}

void insert_row_to_table(struct row* row) {
    uint32_t page_with_free_space_num = row->table->table_header->last_page_general_number;
    enum write_status answer = write_row_to_page(row->table->table_header->db->database_file, page_with_free_space_num, row);
    if (answer != WRITE_OK) printf("Строка не записалась");
}

void select_row_from_table(struct query* query) {
    //проверить что такая колонка есть
    //проверить что тип данных подходит
    //найти начало таблицы 
    //считывать строки и выводить их если подходят по условию
    //
}

void delete_row_from_table(struct row* row) {

}

void update_row_in_table(struct row* row) {

}