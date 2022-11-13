#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include "../include/db.h"

struct database_header;
struct page;
struct table_header;
struct table_schema;
struct row;

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


enum open_status open_file(FILE **in, const char *const filename, const char *const mode);
enum close_status close_file(FILE *in);
enum write_status write_header_to_tech_page(FILE *file, struct page* tech_page, struct page* table_page);
enum write_status overwrite_after_table_delete(FILE *file, struct table_header* deleted_table_header, struct table_header* left_neighnour_header, struct database_header* db_header);
enum write_status write_table_page_first_time(FILE *file, struct page* page_to_write);
enum write_status write_row_to_page(FILE *file, struct page* page_to_write, struct row* row);


#endif