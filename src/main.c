#include <stdio.h>
#include "../include/table.h"
#include "../include/db.h"

void write_db() {
    printf("Test");

    struct table_schema* first_schema = create_table_schema();
    first_schema = add_column_to_schema(first_schema, "age", TYPE_INT32);
    first_schema = add_column_to_schema(first_schema, "male", TYPE_BOOL);
    first_schema = add_string_column_to_schema(first_schema, "name", TYPE_STRING, 20);

    struct database* my_db = get_prepared_database("db.bin", TO_BE_CREATED);

    struct table* table1 = create_table_from_schema(first_schema, "table1", my_db);
    struct table* table2 = create_table_from_schema(first_schema, "table2", my_db);

    struct row* row1 = create_row(table1);

    uint32_t my_age = 20;
    char* my_name = "Nastya";
    bool my_sex = false;

    fill_row_attribute(row1, "age", TYPE_INT32, (void*) &my_age);
    fill_row_attribute(row1, "name", TYPE_STRING, (void*) &my_name);
    fill_row_attribute(row1, "male", TYPE_BOOL, (void*) &my_sex);
    insert_row_to_table(row1); 

    char* changed_name = "Nastya";
    fill_row_attribute(row1, "name", TYPE_STRING, (void*) &changed_name);

    close_database(my_db);

    //delete_table("table1", my_db);
    //delete_table("table2", my_db);
    destroy_column_list(first_schema->columns);
    free(first_schema);

    free(my_db->database_header->first_page);
    free(&my_db->database_header);
    free(my_db);
}

void read_db() {
    struct table_schema* first_schema = create_table_schema();
    first_schema = add_column_to_schema(first_schema, "age", TYPE_INT32);
    first_schema = add_column_to_schema(first_schema, "male", TYPE_BOOL);
    first_schema = add_string_column_to_schema(first_schema, "name", TYPE_STRING, 20);

    struct database* my_db = get_prepared_database("db.bin", EXISTING);
    struct table* my_table = get_table("table2", my_db);
    struct table* my_second_table = get_table("table1", my_db);
}

int main(int argc, char** argv)
{
    //write_db();
    read_db();
    return 0;
}

