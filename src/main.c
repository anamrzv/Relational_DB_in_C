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
    char* my_name = "Dasha";
    bool my_sex = false;

    fill_row_attribute(row1, "age", TYPE_INT32, (void*) &my_age);
    fill_row_attribute(row1, "name", TYPE_STRING, (void*) &my_name);
    fill_row_attribute(row1, "male", TYPE_BOOL, (void*) &my_sex);
    insert_row_to_table(row1); 

    char* changed_name = "Nastya";
    fill_row_attribute(row1, "name", TYPE_STRING, (void*) &changed_name);
    for (size_t i=0; i<20; i++){
        insert_row_to_table(row1);
    } 

    close_database(my_db);

    free(first_schema);
    free(my_db->database_header);
    free(my_db);
}

void read_db() {

    struct database* my_db = get_prepared_database("db.bin", EXISTING);
    
    struct table* my_second_table = get_table("table2", my_db);
    struct table* my_first_table = get_table("table1", my_db);

    struct row* row1 = create_row(my_first_table);
    uint32_t my_age = 21;
    char* my_name = "Dima";
    bool my_sex = true;
    fill_row_attribute(row1, "age", TYPE_INT32, (void*) &my_age);
    fill_row_attribute(row1, "name", TYPE_STRING, (void*) &my_name);
    fill_row_attribute(row1, "male", TYPE_BOOL, (void*) &my_sex);
    insert_row_to_table(row1);

    struct row* row2 = create_row(my_first_table);
    my_age = 10;
    my_name = "Ana";
    my_sex = true;
    fill_row_attribute(row2, "age", TYPE_INT32, (void*) &my_age);
    fill_row_attribute(row2, "name", TYPE_STRING, (void*) &my_name);
    fill_row_attribute(row2, "male", TYPE_BOOL, (void*) &my_sex);
    insert_row_to_table(row2);

    char* changed_name = "Nastya";
    fill_row_attribute(row1, "name", TYPE_STRING, (void*) &changed_name);
    insert_row_to_table(row1);
    
    printf("Query 1\n");
    char* column = "age";
    uint32_t value = 20;
    struct query* select_query = create_query(SELECT_WHERE, my_first_table, column, (void*) &value, -1);
    run_query(select_query);

    printf("Query 2\n");
    char* column2 = "name";
    char* value2 = "Nastya";
    struct query* select_query_2 = create_query(SELECT_WHERE, my_first_table, column2, (void*) &value2, -1);
    run_query(select_query_2);

    printf("Query 3\n");
    char* column3 = "male";
    bool value3 = true;
    struct query* select_query_3 = create_query(SELECT_WHERE, my_first_table, column3, (void*) &value3, -1);
    run_query(select_query_3);

}

int main(int argc, char** argv) {
    write_db();
    read_db();
    return 0;
}

