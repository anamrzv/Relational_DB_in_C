#include <stdio.h>
#include "../include/table.h"
#include "../include/query.h"
#include "../include/db.h"

void write_db() { 
    printf("Test");

    struct table_schema* first_schema = create_table_schema();
    first_schema = add_column_to_schema(first_schema, "id", TYPE_INT32);
    first_schema = add_string_column_to_schema(first_schema, "name", TYPE_STRING, 20);
    first_schema = add_column_to_schema(first_schema, "age", TYPE_INT32);
    first_schema = add_column_to_schema(first_schema, "male", TYPE_BOOL);
    first_schema = add_column_to_schema(first_schema, "eye_color", TYPE_INT32);

    struct table_schema* second_schema = create_table_schema();
    second_schema = add_column_to_schema(second_schema, "id", TYPE_INT32);
    second_schema = add_string_column_to_schema(second_schema, "color", TYPE_STRING, 20);

    struct database* my_db = get_prepared_database("db.bin", TO_BE_CREATED);

    struct table* table1 = create_table_from_schema(first_schema, "people", my_db);
    struct table* table2 = create_table_from_schema(second_schema, "colors", my_db);

    struct row* row1 = create_row(table1);

    uint32_t my_age = 20;
    char* my_name = "Dasha";
    bool my_sex = false;
    uint32_t eye_color = 1;

    uint32_t id = 1;

    
    fill_row_attribute(row1, "name", TYPE_STRING, (void*) &my_name);
    fill_row_attribute(row1, "age", TYPE_INT32, (void*) &my_age);
    fill_row_attribute(row1, "male", TYPE_BOOL, (void*) &my_sex);
    fill_row_attribute(row1, "eye_color", TYPE_INT32, (void*) &eye_color);
    for (size_t i=0; i<100; i++) { //100 даш
        fill_row_attribute(row1, "id", TYPE_INT32, (void*) &id);
        insert_row_to_table(row1);
        id++;
    } 

    char* changed_name = "Nastya";
    eye_color = 2;
    fill_row_attribute(row1, "eye_color", TYPE_INT32, (void*) &eye_color);
    fill_row_attribute(row1, "name", TYPE_STRING, (void*) &changed_name);
    for (size_t i=0; i<100; i++){ //100 насть
        fill_row_attribute(row1, "id", TYPE_INT32, (void*) &id);
        insert_row_to_table(row1);
        id++;
    }

    struct row* colors_row = create_row(table2);
    uint32_t eye_id = 1;
    char* color = "blue";
    fill_row_attribute(colors_row, "id", TYPE_INT32, (void*) &eye_id);
    fill_row_attribute(colors_row, "color", TYPE_STRING, (void*) &color);
    insert_row_to_table(colors_row);

    eye_id = 2;
    color = "grey";
    fill_row_attribute(colors_row, "id", TYPE_INT32, (void*) &eye_id);
    fill_row_attribute(colors_row, "color", TYPE_STRING, (void*) &color);
    insert_row_to_table(colors_row);

    eye_id = 3;
    color = "green";
    fill_row_attribute(colors_row, "id", TYPE_INT32, (void*) &eye_id);
    fill_row_attribute(colors_row, "color", TYPE_STRING, (void*) &color);
    insert_row_to_table(colors_row);

    eye_id = 4;
    color = "brown";
    fill_row_attribute(colors_row, "id", TYPE_INT32, (void*) &eye_id);
    fill_row_attribute(colors_row, "color", TYPE_STRING, (void*) &color);
    insert_row_to_table(colors_row);

    eye_id = 5;
    color = "black";
    fill_row_attribute(colors_row, "id", TYPE_INT32, (void*) &eye_id);
    fill_row_attribute(colors_row, "color", TYPE_STRING, (void*) &color);
    insert_row_to_table(colors_row);

    close_database(my_db);

    free(first_schema);
    free(my_db->database_header);
    free(my_db);
}

void read_db() {

    struct database* my_db = get_prepared_database("db.bin", EXISTING);
    
    struct table* my_first_table = get_table("people", my_db);
    struct table* my_second_table = get_table("colors", my_db);

    struct row* row1 = create_row(my_first_table);
    uint32_t my_age = 21;
    char* my_name = "Dima";
    bool my_sex = true;
    uint32_t eye_color = 3;
    uint32_t id = 201;

    fill_row_attribute(row1, "id", TYPE_INT32, (void*) &id);
    fill_row_attribute(row1, "age", TYPE_INT32, (void*) &my_age);
    fill_row_attribute(row1, "name", TYPE_STRING, (void*) &my_name);
    fill_row_attribute(row1, "male", TYPE_BOOL, (void*) &my_sex);
    fill_row_attribute(row1, "eye_color", TYPE_INT32, (void*) &eye_color);
    insert_row_to_table(row1); // 1 dima

    struct row* row2 = create_row(my_first_table);
    my_age = 10;
    my_name = "Karina";
    my_sex = false;
    eye_color = 4;
    fill_row_attribute(row2, "age", TYPE_INT32, (void*) &my_age);
    fill_row_attribute(row2, "name", TYPE_STRING, (void*) &my_name);
    fill_row_attribute(row2, "male", TYPE_BOOL, (void*) &my_sex);
    fill_row_attribute(row2, "eye_color", TYPE_INT32, (void*) &eye_color);
    for (size_t i=0; i<45; i++){
        id += 1;
        fill_row_attribute(row2, "id", TYPE_INT32, (void*) &id);
        insert_row_to_table(row2); //45 karina
    } 

    char* changed_name = "Nastya";
    eye_color = 5;
    fill_row_attribute(row1, "eye_color", TYPE_INT32, (void*) &eye_color);
    fill_row_attribute(row1, "name", TYPE_STRING, (void*) &changed_name);
    for (size_t i=0; i<50; i++){
        id += 1;
        fill_row_attribute(row1, "id", TYPE_INT32, (void*) &id);
        insert_row_to_table(row1); //50 насть
    }

    my_age = 20;
    my_name = "Nastya";
    uint32_t new_age = 101;

    printf("Query 1: SELECT * FROM table1 WHERE age = 20\n");
    char* column[1] = {"age"};
    void* value[1] = { &my_age };
    struct query* select_query = create_query(SELECT_WHERE, my_first_table, column, value, -1);
    run_query(select_query);

    printf("Query 2: SELECT * FROM table1 WHERE name = Nastya\n");
    char* column2[1] = {"name"};
    void* value2[1] = {&my_name};
    struct query* select_query_2 = create_query(SELECT_WHERE, my_first_table, column2, value2, -1);
    run_query(select_query_2);

    printf("Query 3: SELECT * FROM table1 WHERE male = false\n");
    char* column3[1] = {"male"};
    void* value3[1] = {&my_sex};
    struct query* select_query_3 = create_query(SELECT_WHERE, my_first_table, column3, value3, -1);
    run_query(select_query_3);

    printf("Query 4: UPDATE table1 SET age = 101 WHERE name = Nastya \n");
    char* columns[2] = {"name", "age"};
    void* values[2] = {&my_name, &new_age};
    struct query* select_query_4 = create_query(UPDATE_WHERE, my_first_table, columns, values, -1);
    run_query(select_query_4);

    printf("Query 5: UPDATE table1 SET age = 101 WHERE name = Dasha \n");
    my_name = "Dasha";
    char* columns2[2] = {"name", "age"};
    void* values2[2] = {&my_name, &new_age};
    struct query* select_query_5 = create_query(UPDATE_WHERE, my_first_table, columns2, values2, -1);
    run_query(select_query_5);

    printf("Query 7: SELECT * FROM people JOIN colors ON people.eye_color = colors.id \n");
    struct query_join* select_query_7 = create_query_join(my_first_table, my_second_table, "eye_color", "id");
    run_join_query(select_query_7);

    printf("Query 6: DELETE FROM table1 WHERE name = Dasha \n");
    struct query* select_query_6 = create_query(DELETE_WHERE, my_first_table, column2, value2, -1);
    run_query(select_query_6);

    printf("Конец:)");

}

int main(int argc, char** argv) {
    write_db();
    read_db();
    return 0;
}

