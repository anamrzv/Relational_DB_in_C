#include "../include/table.h"

bool column_exists(const struct column* column_list, const size_t len, const char* name) {
    int32_t index = 0;
    const struct column* cur = column_list;
    if (column_list != NULL){
        while (index != len) {
            if (strcmp(cur->name, name) == 0) return true;
            index++;
            cur=cur->next;
        }
        return false;
    } 
    else return false;
}

size_t column_list_length(const struct column* column_list) {
    size_t s = 0;
    const struct column* pointer = column_list;
    while (pointer!=NULL){
        s++;
        pointer=pointer->next;
    }
    return s;
}

void destroy_column_list(struct column* column_list) {
    struct column* next;
    struct column* cur=column_list;
    while (cur) {
        next = cur->next;
        free(cur);
        cur = next;
    }
}

struct column* column_list_last(struct column* column_list) {
    struct column* cur = column_list;
    if (cur!=NULL) {
        while (cur->next!=NULL) {
        cur=cur->next;
        }
    }
    return cur;
}

void add_back_column(struct column** old, const char* column_name, enum data_type column_type) {
    struct column* new_column = create_column(column_name, column_type);
    if (*old!=NULL) {
        struct column* last_column = column_list_last(*old);
        last_column->next = new_column;
    } 
    else {
        *old = new_column;
    }
}

struct column* delete_column_from_list(struct column* cur, const char* column_name) {
    struct column* next;
    if (cur == NULL) {
        return NULL;
    } else if (strcmp(cur->name, column_name) == 0) {
        next = cur->next;
        free(cur);
        return next;
    } else {
        cur->next = delete_column_from_list(cur->next, column_name);
        return cur;
    }
}



struct column* create_column(const char* column_name, enum data_type column_type ) {
    struct column* created_column = malloc(sizeof (struct column));
    if (created_column == NULL) {
        return NULL;
    }
    strncpy(created_column->name, "", 20);
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
    created_column->next = NULL;
    return created_column;
}

struct table_schema* create_table_schema() {
    struct table_schema* schema = malloc(sizeof(struct table_schema));
    schema->columns = NULL;
    schema->column_count = 0;
    return schema;
}

struct table_schema* add_column_to_schema(struct table_schema* schema, const char* column_name, enum data_type column_type) {
    if (!column_exists(schema->columns, schema->column_count, column_name)) {
        add_back_column(&schema->columns, column_name, column_type);
    } else {
        printf("Колонка с таким названием уже существует");
        return schema;
    }
    schema->column_count += 1;
    
    return schema;
}

struct table_schema* delete_column_from_schema(struct table_schema* schema, const char* column_name) {
    if (column_exists(schema->columns, schema->column_count, column_name)) {
        struct column* new_column_list = delete_column_from_list(schema->columns, column_name);
        schema->columns = new_column_list;
    } else {
        printf("Колонки с таким названием не существует");
        return schema;
    }
    schema->column_count -= 1;

    return schema;
}


