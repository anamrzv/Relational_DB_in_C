#include "../include/table.h"

//создание таблицы

struct table_schema* create_table_schema() {
    struct table_schema* schema = malloc(sizeof(struct table_schema));
    schema->columns = malloc(sizeof(struct column)); //единичка
    schema->column_count = 0;
    return schema;
}

void increase_columns_array(struct table_schema* schema) {
    struct column* old_columns = schema->columns;
    struct column* new_columns = malloc(2*schema->column_count*sizeof(struct column)); //увеличиваем всегда размер в два раза
    for (size_t i=0; i<sizeof(*schema->columns)/sizeof(struct column); i++) { //скопировали из старого в новый
        struct column to_copy = old_columns[i];
        new_columns[i] = to_copy;
    } 
    free(old_columns);
    schema->columns = new_columns;
}

//проверить добавление колонки второй
struct table_schema* add_column_to_schema(struct table_schema* schema, const char* column_name, enum data_type column_type) {
    uint16_t len = sizeof(*schema->columns)/sizeof(struct column);
    if ( len == schema->column_count ) {
        increase_columns_array(schema);
    } 
    struct column* created_column = &schema->columns[schema->column_count];
        strncpy(created_column->name, column_name, strlen(column_name));
        created_column->column_type = column_type;
        switch (column_type) {
            case TYPE_INT32:
                created_column->size = sizeof(int32_t);
                break;
            case TYPE_BOOL:
                created_column->size = sizeof(bool);
                break;
            case TYPE_STRING:
                created_column->size = sizeof(char) * DEFAULT_STRING_LENGTH;
                break;
            case TYPE_FLOAT:
                created_column->size = sizeof(double);
                break;
        }
        schema->column_count += 1;
    return schema;
}
