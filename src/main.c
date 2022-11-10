#include <stdio.h>
#include "../include/table.h"

int main(int argc, char** argv)
{
    printf("Test");

    struct table_schema* first_schema = create_table_schema();
    first_schema = add_column_to_schema(first_schema, "age", TYPE_INT32);
    
    for (size_t k = 0; k<(sizeof(first_schema->columns)/sizeof(struct column)); k++) { //скопировали из старого в новый
        printf("%s ", first_schema->columns[k].name);
        uint16_t a = k;
    } 

    printf("1");

    first_schema = add_column_to_schema(first_schema, "male", TYPE_BOOL);

    for (size_t i=0; i<sizeof(first_schema->columns)/sizeof(struct column); i++) { //скопировали из старого в новый
        printf("%s ", first_schema->columns[i].name);
    } 

    return 0;
}