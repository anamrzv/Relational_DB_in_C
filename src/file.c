#include "../include/file.h"
#include "../include/query.h"

enum open_status open_file(FILE **in, const char *const filename, const char *const mode) {
    *in = fopen(filename, mode);
    if (*in == NULL) return OPEN_ERROR;
    else return OPEN_OK;
}

enum close_status close_file(FILE *in) {
    if (fclose(in) == 0) return CLOSE_OK;
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
    struct page_header* new_page_header = NULL;
    
    if (db_header->last_page_general_number == 1) fseek(file, sizeof(struct database_header), SEEK_SET);
    else fseek(file, (db_header->last_page_general_number-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
    struct page_header* tech_page_header = malloc(sizeof(struct page_header));
    fread(tech_page_header, sizeof(struct page_header), 1, file);
    if (!enough_free_space(tech_page_header, sizeof(struct table_header))) {
        new_page_header = add_tech_page(db_header);
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
        
        if (fwrite(tech_page_header, sizeof(struct page_header), 1, file) == 1) {
            if (new_page_header != NULL) free(new_page_header);
            free(tech_page_header);
            return WRITE_OK;
        }
    }
    if (new_page_header != NULL) free(new_page_header);
    free(tech_page_header);
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

    fseek(file, (page_to_write->page_number_general-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header), SEEK_SET);
    if (fwrite(&size_of_column_array, sizeof(uint16_t), 1, file) != 1) return WRITE_ERROR; //записали размер массива колонок
    fseek(file, (page_to_write->page_number_general-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header)+sizeof(uint16_t), SEEK_SET);
    if (fwrite(columns_array, size_of_column_array, 1, file) != 1) return WRITE_ERROR; //записали массив колонок
    
    free(columns_array);
    free(page_to_write);
    return WRITE_OK;
}

enum write_status write_row_to_page(FILE *file, uint32_t page_to_write_num, struct row* row) {
    struct page_header* new_page_header = NULL;
    uint32_t row_len = row->table->table_header->schema.row_length;
    uint32_t sum_volume = sizeof(struct row_header) + row_len;
    fseek(file, (page_to_write_num-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
    struct page_header* ph_to_write = malloc(sizeof(struct page_header));
    if (fread(ph_to_write, sizeof(struct page_header), 1, file) == 1) {
        if (!enough_free_space(ph_to_write, sum_volume)) {
            new_page_header = add_page(row->table->table_header, row->table->table_header->db->database_header);
            
            fseek(file, (page_to_write_num-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header), SEEK_SET);
            uint16_t size_of_column_array;
            fread(&size_of_column_array, sizeof(uint16_t), 1, file);
            struct column* columns = malloc(size_of_column_array);
            fseek(file, (page_to_write_num-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header)+sizeof(uint16_t), SEEK_SET);
            fread(columns, size_of_column_array, 1, file);

            page_to_write_num = new_page_header->page_number_general;
            ph_to_write = new_page_header;
            ph_to_write->free_bytes -= sizeof(struct page_header)+sizeof(uint16_t)+size_of_column_array;
            ph_to_write->free_space_cursor += sizeof(struct page_header)+sizeof(uint16_t)+size_of_column_array;

            fseek(file, (page_to_write_num-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
            fwrite(ph_to_write, sizeof(struct page_header), 1, file);
            fwrite(&size_of_column_array, sizeof(uint16_t), 1, file);
            fwrite(columns, size_of_column_array, 1, file);
            free(columns);
        }
        fseek(file, (page_to_write_num-1)*DEFAULT_PAGE_SIZE_B + ph_to_write->free_space_cursor, SEEK_SET);
        if (fwrite(row->row_header, sizeof(struct row_header), 1, file) == 1) { 
            fseek(file, (page_to_write_num-1)*DEFAULT_PAGE_SIZE_B + ph_to_write->free_space_cursor+sizeof(struct row_header), SEEK_SET);
            if (fwrite(row->content, row_len, 1, file) == 1) {
                ph_to_write->free_bytes -= sizeof(struct row_header) + row_len;
                ph_to_write->free_space_cursor += sizeof(struct row_header) + row_len;

                fseek(file, (page_to_write_num-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
                if (fwrite(ph_to_write, sizeof(struct page_header), 1, file) != 1) {
                    if (new_page_header != NULL) free(new_page_header);
                    free(ph_to_write);
                    return WRITE_ERROR;
                }
                if (new_page_header != NULL) free(new_page_header);
                free(ph_to_write);
                return WRITE_OK;
            }
        }
        if (new_page_header != NULL) free(new_page_header);
        free(ph_to_write);
        return WRITE_ERROR;
    } else {
        if (new_page_header != NULL) free(new_page_header);
        free(ph_to_write);
        return WRITE_ERROR;
    }    
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
            if (cur->valid && strcmp(cur->name, name) == 0) {
                free(ph);
                return true;
            }

            index++;
            current_pointer += sizeof(struct table_header);
            if (ph->next_page_number_general != 0 && current_pointer == ph->free_space_cursor) {
                current_pointer = 0;
                fseek(file, (ph->next_page_number_general-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
                fread(ph, sizeof(struct page_header), 1, file);
            }
    
        }
        free(ph);
        return false;

    } 
    else { 
        return false;
        free(ph);
    }
}

enum read_status read_columns_of_table(FILE *file, struct table* table) {
    uint16_t* size_of_column_array = malloc(sizeof(uint16_t));
    struct column* columns = malloc(sizeof(struct column)*table->table_header->schema.column_count);
    fseek(file, (table->table_header->first_page_general_number-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header), SEEK_SET);
    fread(size_of_column_array, sizeof(uint16_t), 1, file);
    fread(columns, *size_of_column_array, 1, file);
    table->table_schema->columns = columns;
    table->table_schema->column_count = table->table_header->schema.column_count;
    table->table_schema->last_column = NULL;
    table->table_schema->row_length = table->table_header->schema.row_length;
    free(size_of_column_array);
    return READ_OK;
}

bool compare_int(char* pointer_to_read_row, void* column_value, uint32_t offset) {
    int32_t* value_to_compare = (int32_t*) (pointer_to_read_row + offset);
    int32_t given_value = *((int32_t*) column_value);
    if (*value_to_compare == given_value) return true;
    return false;
}

bool compare_bool(char* pointer_to_read_row, void* column_value, uint32_t offset) {
    bool* value_to_compare = (bool*) (pointer_to_read_row + offset);
    bool given_value = *((bool*) column_value);
    if (*value_to_compare == given_value) return true;
    return false;
}

bool compare_string(char* pointer_to_read_row, void* column_value, uint32_t offset, uint16_t column_size) {
    char* value_to_compare = (char*) pointer_to_read_row + offset; 
    char* given_value = *((char**) column_value);
    if (strcmp(value_to_compare, given_value) == 0) return true;
    return false;
}

bool compare_float(char* pointer_to_read_row, void* column_value, uint32_t offset) {
    double* value_to_compare = (double*) (pointer_to_read_row + offset);
    double given_value = *((double*) column_value);
    if (*value_to_compare == given_value) return true;
    return false;
}

void update_int(char* pointer_to_read_row, void* column_value, uint32_t offset) {
    int32_t* value_to_change = (int32_t*) (pointer_to_read_row + offset);
    int32_t given_value = *((int32_t*) column_value);
    *value_to_change = given_value; 
}

void update_bool(char* pointer_to_read_row, void* column_value, uint32_t offset) {
    bool* value_to_change = (bool*) (pointer_to_read_row + offset);
    bool given_value = *((bool*) column_value);
    *value_to_change = given_value; 
}

void update_string(char* pointer_to_read_row, void* column_value, uint32_t offset, uint16_t column_size) {
    char* value_to_change = (char*) pointer_to_read_row + offset; 
    char* given_value = *((char**) column_value);
    strcpy(value_to_change, given_value);
}

void update_float(char* pointer_to_read_row, void* column_value, uint32_t offset) {
    double* value_to_change = (double*) (pointer_to_read_row + offset);
    double given_value = *((double*) column_value);
    *value_to_change = given_value; 
}

void select_where(FILE *file, struct table* table, uint32_t offset, uint16_t column_size, void* column_value, enum data_type type, int32_t row_count) {
    uint32_t selected_count = 0;
    uint32_t current_pointer = sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*table->table_schema->column_count;
    struct row_header* rh =  malloc(sizeof(struct row_header));
    char* pointer_to_read_row = malloc(table->table_schema->row_length);

    struct page_header* ph = malloc(sizeof(struct page_header));
    fseek(file, (table->table_header->first_page_general_number-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
    fread(ph, sizeof(struct page_header), 1, file); //прочитали заголовок страницы

    fseek(file, (table->table_header->first_page_general_number-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*table->table_schema->column_count, SEEK_SET); //передвинулись на начало строк

    while (current_pointer != ph->free_space_cursor) {

            fseek(file, (ph->page_number_general-1)*DEFAULT_PAGE_SIZE_B+current_pointer, SEEK_SET);
            fread(rh, sizeof(struct row_header), 1, file);
            if (rh->valid) {
                fseek(file, (ph->page_number_general-1)*DEFAULT_PAGE_SIZE_B+current_pointer+sizeof(struct row_header), SEEK_SET);
                fread(pointer_to_read_row, table->table_schema->row_length, 1, file); //прочитали всю строку и у нас есть указатель на нее
                switch (type) {
                    case TYPE_INT32:
                        if (compare_int(pointer_to_read_row, column_value, offset)) {
                            print_passed_content(pointer_to_read_row, table->table_schema->columns, table->table_schema->column_count);
                            selected_count++;
                        }
                        break;
                    case TYPE_BOOL:
                        if (compare_bool(pointer_to_read_row, column_value, offset)) {
                            print_passed_content(pointer_to_read_row, table->table_schema->columns, table->table_schema->column_count);
                            selected_count++;
                        }
                        break;
                    case TYPE_STRING:
                        if (compare_string(pointer_to_read_row, column_value, offset, column_size)) {
                            print_passed_content(pointer_to_read_row, table->table_schema->columns, table->table_schema->column_count);
                            selected_count++;
                        } 
                        break;
                    case TYPE_FLOAT:
                        if (compare_float(pointer_to_read_row, column_value, offset)) {
                            print_passed_content(pointer_to_read_row, table->table_schema->columns, table->table_schema->column_count);
                            selected_count++;
                        }
                        break;
                }
            }

            current_pointer += sizeof(struct row_header)+table->table_schema->row_length;

            if (ph->next_page_number_general != 0 && current_pointer == ph->free_space_cursor) {
                current_pointer = sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*table->table_schema->column_count;
                fseek(file, (ph->next_page_number_general-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
                fread(ph, sizeof(struct page_header), 1, file);
                fseek(file, (ph->page_number_general-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*table->table_schema->column_count, SEEK_SET);
            }
    
        }
    free(rh);
    free(ph);
    free(pointer_to_read_row);
    printf("=====================\n");
    printf("Всего %d строк\n", selected_count);
}

void update_where(FILE *file, struct table* table, struct expanded_query* first, struct expanded_query* second, void** column_values) {
    uint32_t updated_count = 0;
    uint32_t current_pointer = sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*table->table_schema->column_count;

    struct row_header* rh =  malloc(sizeof(struct row_header));
    char* pointer_to_read_row = malloc(table->table_schema->row_length);
    struct page_header* ph = malloc(sizeof(struct page_header));

    fseek(file, (table->table_header->first_page_general_number-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
    fread(ph, sizeof(struct page_header), 1, file); //прочитали заголовок страницы

    fseek(file, (table->table_header->first_page_general_number-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*table->table_schema->column_count, SEEK_SET); //передвинулись на начало строк

        while (current_pointer != ph->free_space_cursor) {

            fseek(file, (ph->page_number_general-1)*DEFAULT_PAGE_SIZE_B+current_pointer, SEEK_SET);
            fread(rh, sizeof(struct row_header), 1, file);
            if (rh->valid) {
                uint32_t pointer_to_update = current_pointer + sizeof(struct row_header);
                fread(pointer_to_read_row, table->table_schema->row_length, 1, file); //прочитали всю строку и у нас есть указатель на нее
                
                switch (first->column_type) {
                    case TYPE_INT32:
                        if (compare_int(pointer_to_read_row, column_values[0], first->offset)) {
                            update_content(pointer_to_read_row, column_values[1], second, table, pointer_to_update, ph->page_number_general);
                            updated_count++;
                        }
                        break;
                    case TYPE_BOOL:
                        if (compare_bool(pointer_to_read_row, column_values[0], first->offset)) {
                            update_content(pointer_to_read_row, column_values[1], second, table, pointer_to_update, ph->page_number_general);
                            updated_count++;
                        }
                        break;
                    case TYPE_STRING:
                        if (compare_string(pointer_to_read_row, column_values[0], first->offset, first->column_size)) {
                            update_content(pointer_to_read_row, column_values[1], second, table, pointer_to_update, ph->page_number_general);
                            updated_count++;
                        }
                        break;
                    case TYPE_FLOAT:
                        if (compare_float(pointer_to_read_row, column_values[0], first->offset)) {
                            update_content(pointer_to_read_row, column_values[1], second, table, pointer_to_update, ph->page_number_general);
                            updated_count++;
                        }
                        break;
                }
                current_pointer += sizeof(struct row_header)+table->table_schema->row_length;
            } else {
                current_pointer += sizeof(struct row_header)+table->table_schema->row_length;
            }

            if (ph->next_page_number_general != 0 && current_pointer == ph->free_space_cursor) {
                current_pointer = sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*table->table_schema->column_count;
                fseek(file, (ph->next_page_number_general-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
                fread(ph, sizeof(struct page_header), 1, file);
                fseek(file, (ph->page_number_general-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*table->table_schema->column_count, SEEK_SET);
            }
    
        }
    free(rh);
    free(ph);
    free(pointer_to_read_row);
    printf("=====================\n");
    printf("Всего %d строк\n", updated_count);
}


void print_int(char* row_start, uint32_t offset) {
    int32_t* value_to_print = (int32_t*) (row_start + offset);
    printf("%" PRId32 ";", *value_to_print);
}

void print_bool(char* row_start, uint32_t offset) {
    bool* value_to_print = (bool*) (row_start + offset);
    printf("%s;", *value_to_print ? "true" : "false");
}

void print_string(char* row_start, uint32_t offset) {
    char* value_to_print = (char*) row_start + offset;
    printf("%s;", value_to_print);
}

void print_float(char* row_start, uint32_t offset) {
    double* value_to_print = (double*) (row_start + offset);
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

void update_content(char* row_start, void* column_value, struct expanded_query* second, struct table* table, uint32_t pointer_to_update, uint32_t page_general_number) {
    switch (second->column_type) {
        case TYPE_INT32:
            update_int(row_start, column_value, second->offset);
            break;
        case TYPE_BOOL:
            update_bool(row_start, column_value, second->offset);
            break;
        case TYPE_STRING:
            update_string(row_start, column_value, second->offset, second->column_size);
            break; 
        case TYPE_FLOAT:
            update_float(row_start, column_value, second->offset);
            break; 
    }

    fseek(table->table_header->db->database_file, (page_general_number-1)*DEFAULT_PAGE_SIZE_B+pointer_to_update, SEEK_SET);
    fwrite(row_start, table->table_schema->row_length, 1, table->table_header->db->database_file);

    print_passed_content(row_start, table->table_schema->columns, table->table_schema->column_count);
}

enum write_status overwrite_previous_last_page_db(FILE *file, struct database_header* db_header, uint32_t new_next_number) {
    uint32_t old_number = db_header->last_page_general_number;
    struct page_header* old = malloc(sizeof(struct page_header));

    if (db_header->last_page_general_number == 1) fseek(file, sizeof(struct database_header), SEEK_SET);
    else fseek(file, (db_header->last_page_general_number-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);

    fread(old, sizeof(struct page_header), 1, file);
    old->next_page_number_general = new_next_number; 

    if (old_number == 1) fseek(file, sizeof(struct database_header), SEEK_SET);
    else fseek(file, (old_number-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
    
    if (fwrite(old, sizeof(struct page_header), 1, file) == 1) {
        free(old);
        return WRITE_OK;
    }
    else {
        free(old);
        return WRITE_ERROR;
    }
}

enum write_status overwrite_previous_last_page(FILE *file, uint32_t previous_last_page_number, uint32_t new_next_number) {
    struct page_header* old = malloc(sizeof(struct page_header));

    fseek(file, (previous_last_page_number-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
    fread(old, sizeof(struct page_header), 1, file);
    old->next_page_number_general = new_next_number; 

    fseek(file, (previous_last_page_number-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
    if (fwrite(old, sizeof(struct page_header), 1, file) == 1) {
        free(old);
        return WRITE_OK;
    }
    else {
        free(old);
        return WRITE_ERROR;
    }
}

void delete_where(FILE *file, struct table* table, struct expanded_query* expanded, void* column_value) {
    uint32_t current_pointer = sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*table->table_schema->column_count;
    uint32_t count_of_deleted = 0;

    struct row_header* rh =  malloc(sizeof(struct row_header));
    char* pointer_to_read_row = malloc(table->table_schema->row_length);
    struct page_header* ph = malloc(sizeof(struct page_header));

    fseek(file, (table->table_header->first_page_general_number-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
    fread(ph, sizeof(struct page_header), 1, file); //прочитали заголовок страницы

    fseek(file, (table->table_header->first_page_general_number-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*table->table_schema->column_count, SEEK_SET); //передвинулись на начало строк

    while (current_pointer != ph->free_space_cursor) {

        fseek(file, (ph->page_number_general-1)*DEFAULT_PAGE_SIZE_B+current_pointer, SEEK_SET);
        fread(rh, sizeof(struct row_header), 1, file);
        if (rh->valid) {
            fseek(file, (ph->page_number_general-1)*DEFAULT_PAGE_SIZE_B+current_pointer+sizeof(struct row_header), SEEK_SET);
            fread(pointer_to_read_row, table->table_schema->row_length, 1, file); //прочитали всю строку и у нас есть указатель на нее
            
            switch (expanded->column_type) {
                case TYPE_INT32:
                    if (compare_int(pointer_to_read_row, column_value, expanded->offset)) {
                        delete_row(pointer_to_read_row, table, current_pointer, ph->page_number_general);
                        count_of_deleted += 1;
                    }
                    break;
                case TYPE_BOOL:
                    if (compare_bool(pointer_to_read_row, column_value, expanded->offset)) {
                        delete_row(pointer_to_read_row, table, current_pointer, ph->page_number_general);
                        count_of_deleted += 1;
                    }
                    break;
                case TYPE_STRING:
                    if (compare_string(pointer_to_read_row, column_value, expanded->offset, expanded->column_size)) {
                        delete_row(pointer_to_read_row, table, current_pointer, ph->page_number_general);
                        count_of_deleted += 1;
                    }
                    break;
                case TYPE_FLOAT:
                    if (compare_float(pointer_to_read_row, column_value, expanded->offset)) {
                        delete_row(pointer_to_read_row, table, current_pointer, ph->page_number_general);
                        count_of_deleted += 1;
                    }
                    break;
            }
        }
                
        current_pointer += sizeof(struct row_header)+table->table_schema->row_length;

        if (ph->next_page_number_general != 0 && current_pointer == ph->free_space_cursor) {
            current_pointer = sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*table->table_schema->column_count;
            fseek(file, (ph->next_page_number_general-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
            fread(ph, sizeof(struct page_header), 1, file);
            fseek(file, (ph->page_number_general-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*table->table_schema->column_count, SEEK_SET);
        }
    }
    free(rh);
    free(ph);
    free(pointer_to_read_row);
    printf("Было удалено %d строк\n", count_of_deleted);
}

void delete_row(char* row_start, struct table* table, uint32_t pointer_to_delete, uint32_t page_general_number) {
    struct row_header rh =  { false };

    fseek(table->table_header->db->database_file, (page_general_number-1)*DEFAULT_PAGE_SIZE_B + pointer_to_delete, SEEK_SET);
    fwrite(&rh, sizeof(struct row_header), 1, table->table_header->db->database_file);
}

void print_joined_content(char* row_start_left, char* row_start_right, struct table* left_table, struct table* right_table, uint32_t left_offset, uint32_t right_offset) {
    uint16_t offset = 0;
    uint16_t second_offset = 0;
    for (size_t i=0; i<left_table->table_schema->column_count; i++) {
        if (offset != left_offset) {
            switch (left_table->table_schema->columns[i].column_type) {
            case TYPE_INT32:
                print_int(row_start_left, offset);
                break;
            case TYPE_BOOL:
                print_bool(row_start_left, offset);
                break;
            case TYPE_STRING:
                print_string(row_start_left, offset);
                break; 
            case TYPE_FLOAT:
                print_float(row_start_left, offset);
                break; 
            }
        } else {
            for (size_t j=0; j<right_table->table_schema->column_count; j++) {
                if (second_offset != right_offset) {
                    switch (right_table->table_schema->columns[j].column_type) {
                        case TYPE_INT32:
                            print_int(row_start_right, second_offset);
                            break;
                        case TYPE_BOOL:
                            print_bool(row_start_right, second_offset);
                            break;
                        case TYPE_STRING:
                            print_string(row_start_right, second_offset);
                            break; 
                        case TYPE_FLOAT:
                            print_float(row_start_right, second_offset);
                            break; 
                    }
                }
                second_offset += right_table->table_schema->columns[j].size;
            }
        }
        offset += left_table->table_schema->columns[i].size;
    }
    printf("\n");
}

bool join_compare_int(char* row_from_left_table, char* row_from_right_table, struct expanded_query* left_expanded, struct expanded_query* right_expanded, struct table* left_table, struct table* right_table) {
    int32_t* left_value = (int32_t*) (row_from_left_table + left_expanded->offset);
    int32_t* right_value = (int32_t*) (row_from_right_table + right_expanded->offset);
    if (*left_value == *right_value) {
        print_joined_content(row_from_left_table, row_from_right_table, left_table, right_table, left_expanded->offset, right_expanded->offset);
        return true;
    } else return false;
}

bool join_compare_bool(char* row_from_left_table, char* row_from_right_table, struct expanded_query* left_expanded, struct expanded_query* right_expanded, struct table* left_table, struct table* right_table) {
    bool* left_value = (bool*) (row_from_left_table + left_expanded->offset);
    bool* right_value = (bool*) (row_from_right_table + right_expanded->offset);
    if (*left_value == *right_value) {
        print_joined_content(row_from_left_table, row_from_right_table, left_table, right_table, left_expanded->offset, right_expanded->offset);
        return true;
    } else return false;
}

bool join_compare_string(char* row_from_left_table, char* row_from_right_table, struct expanded_query* left_expanded, struct expanded_query* right_expanded, struct table* left_table, struct table* right_table) {
    char* left_value = (char*) (row_from_left_table + left_expanded->offset);
    char* right_value = (char*) (row_from_right_table + right_expanded->offset);
    if (strcmp(left_value, right_value) == 0) {
        print_joined_content(row_from_left_table, row_from_right_table, left_table, right_table, left_expanded->offset, right_expanded->offset);
        return true;
    } else return false;
}

bool join_compare_float(char* row_from_left_table, char* row_from_right_table, struct expanded_query* left_expanded, struct expanded_query* right_expanded, struct table* left_table, struct table* right_table) {
    double* left_value = (double*) (row_from_left_table + left_expanded->offset);
    double* right_value = (double*) (row_from_right_table + right_expanded->offset);
    if (*left_value == *right_value) {
        print_joined_content(row_from_left_table, row_from_right_table, left_table, right_table, left_expanded->offset, right_expanded->offset);
        return true;
    } else return false;
}

uint32_t try_connect_with_right_table(FILE *file, struct table* left_table, struct table* right_table, struct expanded_query* left_expanded, struct expanded_query* right_expanded, char* row_from_left_table) {
    uint32_t current_pointer = sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*right_table->table_schema->column_count;
    struct row_header* rh =  malloc(sizeof(struct row_header));
    char* pointer_to_read_row = malloc(right_table->table_schema->row_length);

    struct page_header* ph = malloc(sizeof(struct page_header));
    fseek(file, (right_table->table_header->first_page_general_number-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
    fread(ph, sizeof(struct page_header), 1, file); //прочитали заголовок страницы

    fseek(file, (right_table->table_header->first_page_general_number-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*right_table->table_schema->column_count, SEEK_SET); //передвинулись на начало строк

    while (current_pointer != ph->free_space_cursor) {

            fseek(file, (ph->page_number_general-1)*DEFAULT_PAGE_SIZE_B+current_pointer, SEEK_SET);
            fread(rh, sizeof(struct row_header), 1, file);
            if (rh->valid) {
                fseek(file, (ph->page_number_general-1)*DEFAULT_PAGE_SIZE_B+current_pointer+sizeof(struct row_header), SEEK_SET);
                fread(pointer_to_read_row, right_table->table_schema->row_length, 1, file); //прочитали всю строку и у нас есть указатель на нее
                switch (right_expanded->column_type) {
                    case TYPE_INT32:
                        if (join_compare_int(row_from_left_table, pointer_to_read_row, left_expanded, right_expanded, left_table, right_table)) {
                            return 1;
                        }
                        break;
                    case TYPE_BOOL:
                        if (join_compare_bool(row_from_left_table, pointer_to_read_row, left_expanded, right_expanded, left_table, right_table)) {
                            return 1;
                        }
                        break;
                    case TYPE_STRING:
                        if (join_compare_string(row_from_left_table, pointer_to_read_row, left_expanded, right_expanded, left_table, right_table)) {
                            return 1;
                        } 
                        break;
                    case TYPE_FLOAT:
                        if (join_compare_float(row_from_left_table, pointer_to_read_row, left_expanded, right_expanded, left_table, right_table)) {
                            return 1;
                        }
                        break;
                }
            }

            current_pointer += sizeof(struct row_header)+right_table->table_schema->row_length;

            if (ph->next_page_number_general != 0 && current_pointer == ph->free_space_cursor) {
                current_pointer = sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*right_table->table_schema->column_count;
                fseek(file, (ph->next_page_number_general-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
                fread(ph, sizeof(struct page_header), 1, file);
                fseek(file, (ph->page_number_general-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*right_table->table_schema->column_count, SEEK_SET);
            }
    
        }
    free(rh);
    free(ph);
    free(pointer_to_read_row);
}


void join(FILE *file, struct table* left_table, struct table* right_table, struct expanded_query* left_expanded, struct expanded_query* right_expanded) {
    uint32_t joined_count = 0;
    uint32_t current_pointer = sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*left_table->table_schema->column_count;
    struct row_header* rh =  malloc(sizeof(struct row_header));
    char* pointer_to_read_row = malloc(left_table->table_schema->row_length);

    struct page_header* ph = malloc(sizeof(struct page_header));
    fseek(file, (left_table->table_header->first_page_general_number-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
    fread(ph, sizeof(struct page_header), 1, file); //прочитали заголовок страницы

    fseek(file, (left_table->table_header->first_page_general_number-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*left_table->table_schema->column_count, SEEK_SET); //передвинулись на начало строк

    while (current_pointer != ph->free_space_cursor) {

            fseek(file, (ph->page_number_general-1)*DEFAULT_PAGE_SIZE_B+current_pointer, SEEK_SET);
            fread(rh, sizeof(struct row_header), 1, file);
            if (rh->valid) {
                fseek(file, (ph->page_number_general-1)*DEFAULT_PAGE_SIZE_B+current_pointer+sizeof(struct row_header), SEEK_SET);
                fread(pointer_to_read_row, left_table->table_schema->row_length, 1, file); //прочитали всю строку и у нас есть указатель на нее
                joined_count += try_connect_with_right_table(file, left_table, right_table, left_expanded, right_expanded, pointer_to_read_row);        
            }

            current_pointer += sizeof(struct row_header)+left_table->table_schema->row_length;

            if (ph->next_page_number_general != 0 && current_pointer == ph->free_space_cursor) {
                current_pointer = sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*left_table->table_schema->column_count;
                fseek(file, (ph->next_page_number_general-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
                fread(ph, sizeof(struct page_header), 1, file);
                fseek(file, (ph->page_number_general-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header)+sizeof(uint16_t)+sizeof(struct column)*left_table->table_schema->column_count, SEEK_SET);
            }
    
    }
    free(rh);
    free(ph);
    free(pointer_to_read_row);
    printf("=====================\n");
    printf("Всего %d строк\n", joined_count);

}