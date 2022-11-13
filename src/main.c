#include <stdio.h>
#include "../include/table.h"
#include "../include/db.h"

int main(int argc, char** argv)
{
    printf("Test");

    struct table_schema* first_schema = create_table_schema();
    first_schema = add_column_to_schema(first_schema, "age", TYPE_INT32);

    first_schema = add_column_to_schema(first_schema, "male", TYPE_BOOL);

    first_schema = delete_column_from_schema(first_schema, "age");

    first_schema = add_column_to_schema(first_schema, "age", TYPE_INT32);

    struct database* my_db = get_prepared_database("db6.bin", TO_BE_CREATED);

    struct table* table1 = create_table_from_schema(first_schema, "table1", my_db);
    struct table* table2 = create_table_from_schema(first_schema, "table2", my_db);


    delete_table("table1", my_db);
    delete_table("table2", my_db);
    destroy_column_list(first_schema->columns);
    free(first_schema);

    free(my_db->database_header->first_page);
    free(&my_db->database_header);
    free(my_db);
    return 0;
}