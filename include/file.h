#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include "../include/db.h"

struct database_header;

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
enum write_status write_header_to_db_file(FILE *file, struct database_header* db_header);


#endif