#include "../include/file.h"

enum open_status open_file(FILE **in, const char *const filename, const char *const mode) {
    *in = fopen(filename, mode);
    if (*in == NULL) return OPEN_ERROR;
    else return OPEN_OK;
}

enum close_status close_file(FILE *in) {
    if (fclose(in) == 0)return CLOSE_OK;
    else return CLOSE_ERROR;
}

enum write_status write_db_to_file(FILE *file, struct database_header* db_header, struct page_header* tech_page_header) {
    fseek(file, 0, SEEK_SET);
    if (fwrite(db_header, sizeof(struct database_header), 1, file) == 1) {
        tech_page_header->free_space_cursor += sizeof(struct database_header)+sizeof(struct page_header);
        tech_page_header->free_bytes -= sizeof(struct database_header)+sizeof(struct page_header);
        if (fwrite(tech_page_header, sizeof(struct page_header), 1, file) == 1) return WRITE_OK;
    }
    return WRITE_ERROR;
}

enum write_status write_header_to_tech_page(FILE *file, struct database_header* db_header, struct table_header* new_table_header) {
    if (db_header->last_page_general_number == 1) fseek(file, sizeof(struct database_header), SEEK_SET);
    else fseek(file, (db_header->last_page_general_number-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
    struct page_header* tech_page_header = malloc(sizeof(struct page_header));
    fread(tech_page_header, sizeof(struct page_header), 1, file);
    if (!enough_free_space(tech_page_header, sizeof(struct table_header))) {
        struct page_header* new_page_header = add_tech_page(db_header);
        tech_page_header = new_page_header;
        if (overwrite_dh_after_change(file, db_header) != WRITE_OK) return WRITE_ERROR;

        fseek(file, (tech_page_header->page_number_general-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
        tech_page_header->free_space_cursor += sizeof(struct page_header);
        tech_page_header->free_bytes -= sizeof(struct page_header);
        if (fwrite(tech_page_header, sizeof(struct page_header), 1, file) != 1) return WRITE_ERROR;
    }
    fseek(file, tech_page_header->free_space_cursor, SEEK_SET);
    if (fwrite(new_table_header, sizeof(struct table_header), 1, file) == 1) {
        tech_page_header->free_space_cursor += sizeof(struct table_header);
        tech_page_header->free_bytes -= sizeof(struct table_header);

        if (tech_page_header->page_number_general == 1) fseek(file, sizeof(struct database_header), SEEK_SET);
        else fseek(file, (tech_page_header->page_number_general-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
        
        if (fwrite(tech_page_header, sizeof(struct page_header), 1, file) == 1) return WRITE_OK;
    }
    return WRITE_ERROR;    
}

enum write_status overwrite_th_after_change(FILE *file, struct table_header* changed_table_header) {
    fseek(file, sizeof(struct database_header) + sizeof(struct page_header) + (changed_table_header->number_in_tech_page-1)*sizeof(struct table_header), SEEK_SET);
    if (fwrite(changed_table_header, sizeof(struct table_header), 1, file) == 1) return WRITE_OK;
    else return WRITE_ERROR;
}

enum write_status overwrite_dh_after_change(FILE *file, struct database_header* changed_db_header) {
    fseek(file, 0, SEEK_SET);
    if (fwrite(changed_db_header, sizeof(struct database_header), 1, file) == 1) return WRITE_OK;
    else return WRITE_ERROR;
}

enum write_status write_table_page(FILE *file, struct page_header* page_to_write, struct table_schema* schema) {
    fseek(file, (page_to_write->page_number_general-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
    uint16_t size_of_column_array = schema->column_count*sizeof(struct column);

    page_to_write->free_bytes -= sizeof(struct page_header)+sizeof(uint16_t)+size_of_column_array;
    page_to_write->free_space_cursor += sizeof(struct page_header)+sizeof(uint16_t)+size_of_column_array;
    if (fwrite(page_to_write, sizeof(struct page_header), 1, file) != 1) return WRITE_ERROR;

    struct column* columns_array = malloc(size_of_column_array);

    struct column* cur = schema->columns;
    for (size_t i=0; i<schema->column_count; i++){
        columns_array[i] = *cur;
        cur = cur->next;
    }

    if (fwrite(&size_of_column_array, sizeof(uint16_t), 1, file) != 1) return WRITE_ERROR; //записали размер массива колонок
    if (fwrite(columns_array, size_of_column_array, 1, file) != 1) return WRITE_ERROR; //записали массив колонок
    free(columns_array);
    return WRITE_OK;
}

enum write_status write_row_to_page(FILE *file, uint32_t page_to_write_num, struct row* row) {
    uint32_t row_len = row->table->table_header->schema.row_length;
    uint32_t sum_volume = sizeof(struct row_header) + row_len;
    fseek(file, (page_to_write_num-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
    struct page_header* ph_to_write = malloc(sizeof(struct page_header));
    if (fread(ph_to_write, sizeof(struct page_header), 1, file) == 1) {
        if (!enough_free_space(ph_to_write, sum_volume)) {
            struct page_header* new_page_header = add_page(row->table->table_header, row->table->table_header->db->database_header);
            if (overwrite_dh_after_change(file, row->table->table_header->db->database_header) != WRITE_OK) return WRITE_ERROR;
            
            fseek(file, (page_to_write_num-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header), SEEK_SET);
            uint16_t size_of_column_array;
            fread(&size_of_column_array, sizeof(uint16_t), 1, file);
            struct column* columns;
            fread(columns, size_of_column_array, 1, file);

            page_to_write_num = new_page_header->page_number_general;
            ph_to_write = new_page_header;
            ph_to_write->free_bytes -= sizeof(struct page_header)+sizeof(uint16_t)+size_of_column_array;
            ph_to_write->free_space_cursor += sizeof(struct page_header)+sizeof(uint16_t)+size_of_column_array;

            fseek(file, (page_to_write_num-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
            if (fwrite(ph_to_write, sizeof(struct page_header), 1, file) != 1) return WRITE_ERROR;
            if (fwrite(&size_of_column_array, sizeof(uint16_t), 1, file) != 1) return WRITE_ERROR;
            if (fwrite(columns, size_of_column_array, 1, file) != 1) return WRITE_ERROR;
        }
        fseek(file, (page_to_write_num-1)*DEFAULT_PAGE_SIZE_B + ph_to_write->free_space_cursor, SEEK_SET);
        if (fwrite(row->row_header, sizeof(struct row_header), 1, file) == 1) { 
            if (fwrite(row->content, row_len, 1, file) == 1) {
                ph_to_write->free_bytes -= sizeof(struct row_header) + row_len;
                ph_to_write->free_space_cursor += sizeof(struct row_header) + row_len;

                fseek(file, (page_to_write_num-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
                if (fwrite(ph_to_write, sizeof(struct page_header), 1, file) != 1) return WRITE_ERROR;
                return WRITE_OK;
            }
        }
        return WRITE_ERROR;
    } else return WRITE_ERROR;
    
}

enum read_status read_database_header(FILE *file, struct database_header* db_header) {
   fseek(file, 0, SEEK_SET);
   if (fread(db_header, sizeof(struct database_header), 1, file) == 1) return READ_OK;
   else return READ_ERROR;
}

enum read_status read_table_header(FILE *file, const char *const tablename, struct table_header* read_th, size_t table_count) {
    if (table_exists(file, table_count, tablename, read_th)) return READ_OK;
    else return READ_ERROR;
}

bool table_exists(FILE *file, const size_t len, const char* name, struct table_header* cur) {
    uint32_t index = 0;
    uint32_t current_pointer = sizeof(struct database_header)+sizeof(struct page_header);

    struct page_header* ph = malloc(sizeof(struct page_header));
    fseek(file, sizeof(struct database_header), SEEK_SET);
    fread(ph, sizeof(struct page_header), 1, file);

    if (len != 0) {
        fseek(file, sizeof(struct database_header)+sizeof(struct page_header), SEEK_SET);

        while (index != len) {

            fread(cur, sizeof(struct table_header), 1, file);
            if (cur->valid && strcmp(cur->name, name) == 0) return true;

            index++;
            current_pointer += sizeof(struct table_header);
            if (ph->next_page_number_general != 0 && current_pointer == ph->free_space_cursor) {
                current_pointer = 0;
                fseek(file, (ph->next_page_number_general-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
                fread(ph, sizeof(struct page_header), 1, file);
            }
    
        }

        return false;

    } 
    else return false;
}

enum read_status read_columns_of_table(FILE *file, struct table* table) {
    fseek(file, (table->table_header->first_page_general_number-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header), SEEK_SET);
    uint16_t size_of_column_array;
    fread(&size_of_column_array, sizeof(uint16_t), 1, file);
    struct column* columns = malloc(sizeof(struct column)*table->table_header->schema.column_count);
    fread(columns, size_of_column_array, 1, file);
    table->table_schema->columns = columns;
    table->table_schema->column_count = table->table_header->schema.column_count;
    table->table_schema->last_column = NULL;
    table->table_schema->row_length = table->table_header->schema.row_length;
    return READ_OK;
}

bool compare_int(char* pointer_to_read_row, void* column_value, uint32_t offset) {
    int32_t* value_to_compare = (int32_t*) pointer_to_read_row + offset; //по этому адресу лежит чиселко
    int32_t given_value = *((int32_t*) column_value);
    if (*value_to_compare == given_value) return true;
    return false;
}

bool compare_bool(char* pointer_to_read_row, void* column_value, uint32_t offset) {
    bool* value_to_compare = (bool*) pointer_to_read_row + offset;
    bool given_value = *((bool*) column_value);
    if (*value_to_compare == given_value) return true;
    return false;
}

bool compare_string(char* pointer_to_read_row, void* column_value, uint32_t offset, uint16_t column_size) {
    char* value_to_compare = (char*) pointer_to_read_row + offset; 
    //printf("\nБыло %s", value_to_compare);
    //printf("\ndlina %d", strlen(value_to_compare));
    char* given_value = *((char**) column_value);

    // char* dest = malloc(sizeof(char)*column_size);
    // strncpy(dest, "", column_size);
    // strncpy(dest, given_value, strlen(given_value));
    // printf("\nСтало %s", dest);
    
    //strncpy(dest, value_to_compare, column_size);
    if (strcmp(value_to_compare, given_value) == 0) return true;
    return false;
}

bool compare_float(char* pointer_to_read_row, void* column_value, uint32_t offset) {
    double* value_to_compare = (double*) pointer_to_read_row + offset;
    double given_value = *((double*) column_value);
    if (*value_to_compare == given_value) return true;
    return false;
}

//TODO table num in tech page поправить
void select_where(FILE *file, struct table* table, uint32_t offset, uint16_t column_size, void* column_value, enum data_type type, int32_t row_count) {
    //считывать строки и выводить их если подходят по условию
    uint32_t current_pointer = sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*table->table_schema->column_count;
    struct row_header* rh =  malloc(sizeof(struct row_header));
    char* pointer_to_read_row = malloc(table->table_schema->row_length);

    struct page_header* ph = malloc(sizeof(struct page_header));
    fseek(file, (table->table_header->first_page_general_number-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
    fread(ph, sizeof(struct page_header), 1, file); //прочитали заголовок страницы

    fseek(file, sizeof(uint16_t)+sizeof(struct column)*table->table_schema->column_count, SEEK_CUR); //передвинулись на начало строк

        while (current_pointer != ph->free_space_cursor) {

            fread(rh, sizeof(struct row_header), 1, file);
            if (rh->valid) {
                fread(pointer_to_read_row, table->table_schema->row_length, 1, file); //прочитали всю строку и у нас есть указатель на нее
                switch (type) {
                    case TYPE_INT32:
                        if (compare_int(pointer_to_read_row, column_value, offset)) print_passed_content(pointer_to_read_row, table->table_schema->columns, table->table_schema->column_count);
                        break;
                    case TYPE_BOOL:
                        if (compare_bool(pointer_to_read_row, column_value, offset)) print_passed_content(pointer_to_read_row, table->table_schema->columns, table->table_schema->column_count);
                        break;
                    case TYPE_STRING:
                        if (compare_string(pointer_to_read_row, column_value, offset, column_size)) print_passed_content(pointer_to_read_row, table->table_schema->columns, table->table_schema->column_count);
                        break;
                    case TYPE_FLOAT:
                        if (compare_float(pointer_to_read_row, column_value, offset)) print_passed_content(pointer_to_read_row, table->table_schema->columns, table->table_schema->column_count);
                        break;
                }
            }

            current_pointer += sizeof(struct row_header)+table->table_schema->row_length;

            if (ph->next_page_number_general != 0 && current_pointer == ph->free_space_cursor) {
                current_pointer = sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*table->table_schema->column_count;
                fseek(file, (ph->next_page_number_general-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
                fread(ph, sizeof(struct page_header), 1, file);
                fseek(file, sizeof(uint16_t)+sizeof(struct column)*table->table_schema->column_count, SEEK_CUR);
            }
    
        }
}

void print_int(char* row_start, uint32_t offset) {
    int32_t* value_to_print = (int32_t*) row_start + offset;
    printf("%" PRId32 ";", *value_to_print);
}

void print_bool(char* row_start, uint32_t offset) {
    bool* value_to_print = (bool*) row_start + offset;
    printf("%s;", *value_to_print ? "true" : "false");
}

void print_string(char* row_start, uint32_t offset) {
    char* value_to_print = (char*) row_start + offset;
    printf("%s;", value_to_print);
}

void print_float(char* row_start, uint32_t offset) {
    double* value_to_print = (double*) row_start + offset;
    printf("%f;", *value_to_print);
}

void print_passed_content(char* row_start, struct column* columns, uint16_t len) {
    uint16_t offset = 0;
    for (size_t i=0; i<len; i++) {
        switch (columns[i].column_type) {
            case TYPE_INT32:
                print_int(row_start, offset);
                break;
            case TYPE_BOOL:
                print_bool(row_start, offset);
                break;
            case TYPE_STRING:
                print_string(row_start, offset);
                break; 
            case TYPE_FLOAT:
                print_float(row_start, offset);
                break; 
        }
        offset += columns[i].size;
    }
    printf("\n");
}