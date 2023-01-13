#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include "db.h"

enum data_type;
struct database_header;
struct page;
struct table_header;
struct table_schema;
struct row;
struct page_header;
struct column;
struct table;
struct expanded_query;

enum open_status  {
  OPEN_OK = 0,
  OPEN_ERROR
};

enum close_status  {
  CLOSE_OK = 0,
  CLOSE_ERROR
};

enum write_status {
  WRITE_OK = 0,
  WRITE_ERROR
};

enum read_status {
  READ_OK = 0,
  READ_ERROR
};


enum open_status open_file(FILE **in, const char *const filename, const char *const mode);
enum close_status close_file(FILE *in);

enum write_status write_db_to_file(FILE *file, struct database_header* db_header, struct page_header* tech_page_header);
enum write_status write_header_to_tech_page(FILE *file, struct database_header* db_header, struct table_header* table_header);
enum write_status overwrite_th_after_change(FILE *file, struct table_header* changed_table_header);
enum write_status overwrite_dh_after_change(FILE *file, struct database_header* changed_db_header);
enum write_status write_table_page(FILE *file, struct page_header* page_to_write, struct table_schema* schema);
enum write_status write_row_to_page(FILE *file, uint32_t page_to_write_num, struct row* row);

enum read_status read_database_header(FILE *file, struct database_header* db_header);
enum read_status read_table_header(FILE *file, const char *const tablename, struct table_header* read_th, size_t table_count);
bool table_exists(FILE *file, const size_t len, const char* name, struct table_header* cur);

enum read_status read_columns_of_table(FILE *file, struct table* table);
bool compare_int(char* pointer_to_read_row, void* column_value, uint32_t offset);
bool compare_bool(char* pointer_to_read_row, void* column_value, uint32_t offset);
bool compare_string(char* pointer_to_read_row, void* column_value, uint32_t offset, uint16_t column_size);
bool compare_float(char* pointer_to_read_row, void* column_value, uint32_t offset);

void update_int(char* pointer_to_read_row, void* column_value, uint32_t offset);
void update_bool(char* pointer_to_read_row, void* column_value, uint32_t offset);
void update_string(char* pointer_to_read_row, void* column_value, uint32_t offset, uint16_t column_size);
void update_float(char* pointer_to_read_row, void* column_value, uint32_t offset);

void select_where(FILE *file, struct table* table, uint32_t offset, uint16_t column_size, void* column_value, enum data_type type, int32_t row_count);
void update_where(FILE *file, struct table* table, struct expanded_query* first, struct expanded_query* second, void** column_values);


void print_int(char* row_start, uint32_t offset);
void print_bool(char* row_start, uint32_t offset);
void print_string(char* row_start, uint32_t offset);
void print_float(char* row_start, uint32_t offset);
void print_passed_content(char* row_start, struct column* columns, uint16_t len);

void update_content(char* row_start, void* column_value, struct expanded_query* second, struct table* table, uint32_t pointer_to_update, uint32_t page_general_number);
enum write_status overwrite_previous_last_page_db(FILE *file, struct database_header* db_header, uint32_t new_next_number);
enum write_status overwrite_previous_last_page(FILE *file, uint32_t previous_last_page_number, uint32_t new_next_number);

void delete_row(char* row_start, struct table* table, uint32_t pointer_to_delete, uint32_t page_general_number);
void delete_where(FILE *file, struct table* table, struct expanded_query* expanded, void* column_value);

void print_joined_content(char* row_start_left, char* row_start_right, struct table* left_table, struct table* right_table, uint32_t left_offset, uint32_t right_offset);
bool join_compare_int(char* row_from_left_table, char* row_from_right_table, struct expanded_query* left_expanded, struct expanded_query* right_expanded, struct table* left_table, struct table* right_table);
bool join_compare_bool(char* row_from_left_table, char* row_from_right_table, struct expanded_query* left_expanded, struct expanded_query* right_expanded, struct table* left_table, struct table* right_table);
bool join_compare_string(char* row_from_left_table, char* row_from_right_table, struct expanded_query* left_expanded, struct expanded_query* right_expanded, struct table* left_table, struct table* right_table);
bool join_compare_float(char* row_from_left_table, char* row_from_right_table, struct expanded_query* left_expanded, struct expanded_query* right_expanded, struct table* left_table, struct table* right_table);
uint32_t try_connect_with_right_table(FILE *file, struct table* left_table, struct table* right_table, struct expanded_query* left_expanded, struct expanded_query* right_expanded, char* row_from_left_table);
void join(FILE *file, struct table* left_table, struct table* right_table, struct expanded_query* left_expanded, struct expanded_query* right_expanded);

#endif