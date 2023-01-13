## Модуль, реализующий хранение в одном файле данных (выборку, размещение и гранулярное обновление) информации общим объёмом от 10GB.
### Вариант: Реляционные таблицы


#### 1. Цели и описание варианта
Создать модуль, реализующий хранение в одном файле данных в виде строк в таблицах общим объёмом от 10GB, имеющий вид реляционной БД
| Организация элементов данных | Способ реализации отношений | Примеры | Состав схемы данных | Состав модели фильтра данных | Примеры языков запросов |
| :---: | :---: | :---:   | :---: | :---: | :---:   | 
| Таблицы записей, несущих поля | Не материализованы, через выражения запросов по значениям | РСУБД, метаданные бинарных исполняемых модулей | Виды записей таблиц, виды значений в полях | Условия по содержимому элементов данных и отношениям между ними | SQL, LINQ |

#### 2. Задачи
* Спроектировать структуры данных для представления информации в оперативной памяти
  - Для порции данных. Поддержать тривиальные значения по меньшей мере следующих типов: цетырёхбайтовые целые числа и числа с плавающей точкой, текстовые строки произвольной длины, булевские значения
  - Для информации о запросе
* Спроектировать представление данных с учетом схемы для файла данных и реализовать базовые операции для работы с ним:
  - Операции над схемой данных (создание и удаление колонок)
  - Базовые операции над элементами данных в соответствии с текущим состоянием схемы (над записями заданного вида)
    1. Вставка элемента данных
    2. Перечисление элементов данных
    3. Обновление элемента данных
    4. Удаление элемента данных
* Используя в сигнатурах только структуры данных из п.1, реализовать публичный интерфейс со следующими операциями над файлом данных:
  - Добавление, удаление и получение информации о элементах схемы данных, размещаемых в файле данных, на уровне, соответствующем виду записей
  - Добавление нового элемента данных определённого вида
  - Выборка набора элементов данных с учётом заданных условий и отношений со смежными элементами данных (по свойствам/полями/атрибутам и логическим связям соответственно)
  - Обновление элементов данных, соответствующих заданным условиям
  - Удаление элементов данных, соответствующих заданным условиям
* Реализовать тестовую программу для демонстрации работоспособности решения

#### 3. Описание работы

Программа логически разделена на 4 модуля:

- **db** (операции с БД: работа со страницами, получение БД и таблиц, запросов)
- **file** (работа с файлом БД: все операции чтения/записи, реализация запросов)
- **table_schema** (операции над схемой данных)
- **table_content** (операции над элементами данных)

##### Подключение к БД
Один из двух способов: создание новой БД или подключение к существующей
```c
struct database* my_db = get_prepared_database("db.bin", TO_BE_CREATED);
struct database* my_db = get_prepared_database("db.bin", EXISTING);
```
```c
struct database_header {
    char name[MAX_DB_NAME_LEN];
    struct database* db;
    uint32_t table_count;
    uint32_t page_count;
    uint32_t page_size; 
    uint32_t last_page_general_number;
};

struct database {
    struct database_header* database_header;
    FILE* database_file;
};

struct page_header {
    uint16_t free_bytes;
    uint32_t page_number_general;
    bool dirty;
    uint32_t free_space_cursor; 
    char table_name[MAX_TABLE_NAME_LEN];
    uint32_t table_number_in_tech_page;
    uint32_t next_page_number_general;
};
```

##### Создание схемы таблицы
```c
struct table_schema* first_schema = create_table_schema();
    first_schema = add_column_to_schema(first_schema, "id", TYPE_INT32);
    first_schema = add_string_column_to_schema(first_schema, "name", TYPE_STRING, 20);
    first_schema = add_column_to_schema(first_schema, "age", TYPE_INT32);
    first_schema = add_column_to_schema(first_schema, "male", TYPE_BOOL);
    first_schema = add_column_to_schema(first_schema, "eye_color", TYPE_INT32);
```
```c
struct table_schema {
    uint16_t column_count;
    struct column* columns; 
    struct column* last_column; 
    uint64_t row_length;
};

struct column {
    char name[MAX_NAME_LEN];
    enum data_type column_type;
    uint32_t size;
    struct column* next;
};

enum data_type {
    TYPE_INT32 = 0,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_FLOAT
};
```

##### Создание новой таблицы из схемы и чтение существующей таблицы из БД
```c
struct table* table = create_table_from_schema(first_schema, "people", my_db);
struct table* my_first_table = get_table("people", my_db);
```
```c
struct table_header {
    char name[MAX_NAME_LEN];
    struct database* db;
    struct table* table;
    uint32_t page_count;
    uint32_t number_in_tech_page;
    bool valid;
    struct table_schema schema;
    uint32_t first_page_general_number;
    uint32_t last_page_general_number;
};

struct table {
    struct table_header* table_header;
    struct table_schema* table_schema;
};
```

##### Создание строки таблицы и заполнение данными
```c
struct row* first_row = create_row(table);

uint32_t my_age = 20;
char* my_name = "Dasha";
bool my_sex = false;
uint32_t eye_color = 1;
uint32_t id = 1;

fill_row_attribute(first_row, "name", TYPE_STRING, (void*) &my_name);
fill_row_attribute(first_row, "age", TYPE_INT32, (void*) &my_age);
fill_row_attribute(first_row, "male", TYPE_BOOL, (void*) &my_sex);
fill_row_attribute(first_row, "eye_color", TYPE_INT32, (void*) &eye_color);
fill_row_attribute(row1, "id", TYPE_INT32, (void*) &id);
```
```c
struct row_header {
    bool valid;
};

struct row {
    struct row_header* row_header;
    struct table* table;
    void** content;
};
```

##### Вставка строки
```c
insert_row_to_table(first_row);
```

##### Создание и выполнение запросов
```c
// Query 1: SELECT * FROM people WHERE age = 20
char* column[1] = {"age"};
void* value[1] = { &my_age };
struct query* select_query = create_query(SELECT_WHERE, my_first_table, column, value, -1);
run_query(select_query);

// Query 2: UPDATE people SET age = 101 WHERE name = Nastya
char* columns[2] = {"name", "age"};
void* values[2] = {&my_name, &new_age};
struct query* update_query = create_query(UPDATE_WHERE, my_first_table, columns, values, -1);
run_query(update_query);

//Query 3: SELECT * FROM people JOIN colors ON people.eye_color = colors.id
struct query_join* join_query = create_query_join(my_first_table, my_second_table, "eye_color", "id");
run_join_query(join_query);

//Query 4: DELETE FROM people WHERE name = Nastya
char* delete_column[1] = {"name"};
void* delete_value[1] = {&my_name};
struct query* delete_query = create_query(DELETE_WHERE, my_first_table, delete_column, delete_value, -1);
run_query(delete_query);
```
```c
struct query {
    enum query_type q_type;
    struct table* table;
    char** column_name;
    void** column_value;
    int32_t rows_number;
};

struct query_join {
    struct table* left_table;
    struct table* right_table;
    char* left_column_name;
    char* right_column_name;
};

enum query_type {
    SELECT_WHERE = 0,
    UPDATE_WHERE,
    DELETE_WHERE,
};
```

Пример вывода команд:

```c
1;Dasha;20;false;blue;
2;Nastya;20;false;grey;
3;Dima;21;true;green;
=====================
Всего 3 строк
```

#### 4. Аспекты реализации

Принципы работы модуля следующие:

1. Одна база данных отражается в один файл. Помимо файла, с БД связан заголовок БД, в котором хранится ее название, размер страниц (по умолчанию 4 KBytes), количество страниц, количество таблиц и номер последней страницы.
2. Файл БД логически поделен на страницы одинакового размера. При создании новой страницы, в ее начало записывается заголовок страницы: ее номер, информация о ее заполненности, информация о таблице, которой она принадлежит или о БД в случае служебной страницы. Страница может принадлежать либо таблице, либо БД (служебная страница). Каждая страница принадлежит только одной таблице/БД.
3. Первая страница в файле БД - служебная. При создании БД в нее записываются заголовок БД, заголовок служебной таблицы. При нехватке места для записи информации о новых таблицах, создается новая служебная страница в конце файла. 
4. Для создания таблицы предварительно нужно создать схему таблицы. Схема таблицы содержит в себе описания колонок: их названия, тип данных, размер в байтах. Также в схеме хранится длина строки в байтах, к которой мы обращамся для считывании строк.
5. У таблицы должен быть заголовок и схема. При создании таблицы идет ее запись в файл БД. Во-первых, создается заголовок таблицы и записывается в служебную страницу в конец. Заголовок таблицы содержит информацию о названии таблицы, номерах ее первой и последней страницы, валидности таблицы. Во-вторых, создается первая страница таблицы в конце файла. В эту страницу записывается заголовок страницы (его величина одинакова для всех таблиц), длина в байтах информации о колонках таблицы, а дальше идут данные переменной длины: информация о колонках, из которых состоит схема. Благодаря хранению в начале страницы длины информации о колонках, мы при считывании таблицы с БД знаем, сколько байт надо считать, чтобы прочитать информацию о колонках, т.к. эта длина для таблиц разная. Эта информация - заголовок, длина колонок, колонки - дублируется в последующие страницы таблицы, которые создаются, если не хватаем местра в странице для записи данных. Перемещение по страницам переходит по их порядковым номерам.
```c
fseek(file, (page_to_write_num-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
```
```c
enum write_status write_table_page(FILE *file, struct page_header* page_to_write, struct table_schema* schema) {
    fseek(file, (page_to_write->page_number_general-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
    uint16_t size_of_column_array = schema->column_count*sizeof(struct column);

    page_to_write->free_bytes -= sizeof(struct page_header)+sizeof(uint16_t)+size_of_column_array;
    page_to_write->free_space_cursor += sizeof(struct page_header)+sizeof(uint16_t)+size_of_column_array;
    if (fwrite(page_to_write, sizeof(struct page_header), 1, file) != 1) return WRITE_ERROR;

    struct column* columns_array = malloc(size_of_column_array);

    struct column* cur = schema->columns;
    for (size_t i=0; i<schema->column_count; i++){
        columns_array[i] = *cur;
        cur = cur->next;
    }

    fseek(file, (page_to_write->page_number_general-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header), SEEK_SET);
    if (fwrite(&size_of_column_array, sizeof(uint16_t), 1, file) != 1) return WRITE_ERROR; //записали размер массива колонок
    fseek(file, (page_to_write->page_number_general-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header)+sizeof(uint16_t), SEEK_SET);
    if (fwrite(columns_array, size_of_column_array, 1, file) != 1) return WRITE_ERROR; //записали массив колонок
    
    free(columns_array);
    free(page_to_write);
    return WRITE_OK;
}
```
```c
struct table* get_table(const char* tablename, struct database* db) {
    struct table* new_table = malloc(sizeof(struct table));
    struct table_header* new_th = malloc(sizeof(struct table_header));
    struct table_schema* new_ts = malloc(sizeof(struct table_schema));

    if (read_table_header(db->database_file, tablename, new_th, db->database_header->table_count) == READ_OK) {
        new_table->table_header = new_th;
        new_table->table_schema = new_ts;
        read_columns_of_table(db->database_file, new_table);
        new_table->table_header->db = db;
        return new_table;
    } else {
        printf("Не удалось прочитать таблицу\n");
        return NULL;
    }
}

enum read_status read_columns_of_table(FILE *file, struct table* table) {
    uint16_t* size_of_column_array = malloc(sizeof(uint16_t));
    struct column* columns = malloc(sizeof(struct column)*table->table_header->schema.column_count);
    fseek(file, (table->table_header->first_page_general_number-1)*DEFAULT_PAGE_SIZE_B+sizeof(struct page_header), SEEK_SET);
    fread(size_of_column_array, sizeof(uint16_t), 1, file);
    fread(columns, *size_of_column_array, 1, file);
    table->table_schema->columns = columns;
    table->table_schema->column_count = table->table_header->schema.column_count;
    table->table_schema->last_column = NULL;
    table->table_schema->row_length = table->table_header->schema.row_length;
    free(size_of_column_array);
    return READ_OK;
}
```
6. Все остальное место в странице оставлено для данных: заголовков строк и самих строк. 
7. После того, как мы создали строку определенной таблицы исходя из ее схемы, следует заполнение ее данными. Для заполнения данными мы передаем имя колнки, ее тип, и указатель типа void* на данные. Это позволяет сделать метод заполнения generic. Определение места в строке, куда должна произвестись запись, происходит в помощью вычисления offset колонки за счет хранения длин колонок в схеме.
```c
void fill_row_attribute(struct row* row, char* column_name, enum data_type column_type, void* value) {
    read_columns_of_table(row->table->table_header->db->database_file, row->table);
    uint32_t offset = column_offset(row->table->table_schema->columns, row->table->table_schema->column_count, column_name);
    if (offset != -1) {
        switch (column_type) {
            case TYPE_INT32:
                fill_with_int(row, *((int32_t*) value), offset);
                break;
            case TYPE_BOOL:
                fill_with_bool(row, *((bool*) value), offset);
                break;
            case TYPE_STRING:
                fill_with_string(row, *((char**) value), offset, string_column_length(row->table->table_schema->columns, row->table->table_schema->column_count, column_name));
                break;
            case TYPE_FLOAT:
                fill_with_float(row, *((double*) value), offset);
                break;
        }
    } else printf("Невозможно заполнить строку: Колонки с таким названием атибута не существует\n");   
}

void fill_with_int(struct row* row, int32_t value, uint32_t offset) {
    char* pointer_to_write = (char*) row->content + offset;
    *((int32_t*) (pointer_to_write)) = value;
}

void fill_with_bool(struct row* row, bool value, uint32_t offset) {
    char* pointer_to_write = (char*) row->content + offset;
    *((bool*) (pointer_to_write)) = value;
}

void fill_with_string(struct row* row, char* value, uint32_t offset, uint32_t string_len) {
    if (string_len != -1) {
        char* pointer_to_write = (char*) row->content + offset;
        strncpy(pointer_to_write, "", string_len);
        strncpy(pointer_to_write, value, strlen(value));
    }
}

void fill_with_float(struct row* row, double value, uint32_t offset) {
    char* pointer_to_write = (char*) row->content + offset;
    *((double*) (pointer_to_write)) = value;
}
```
```c
int32_t column_offset(const struct column* column_list, const size_t len, char* name) {
    int32_t index = 0;
    uint32_t offset = 0;
    if (column_list != NULL){
        while (index != len) {
            struct column col = column_list[index]; 
            uint32_t b = column_list[index].size;
            if (strcmp(column_list[index].name, name) == 0) return offset;
            index++;
            offset += b;
        }
        return -1;
    } 
    else return -1;
}
```
8. При вставке строк, если в последней странице таблицы не достаточно места, будет автоматически создана новая страница таблицы.
9. При каждом большом изменении БД: добавлении таблицы, появления новых страниц - обновляются заголовки БД и таблиц.
10. Для доступа к данным нужно создавать и запускать запросы. В зависимости от типа запроса будет выбрана соответствующая логика. Запросы получают на вход таблиц_ы, названи_я колонок, значени_я переменных, котор_ые использу_ются в запросе. Причем значения переменных представлены как массив void*, что позволяет передавать данные любого типа. По названию колонки будет определен тип переменной и выбрана соответствующая логика.

#### 4. Результаты

Операция вставки выполняется за O(1) независимо от размера данных, представленных в файле
![image](https://user-images.githubusercontent.com/79102850/204132136-01acc632-7e31-4d33-a49d-a76ac7b056eb.png)

Операция выборки без учёта отношений (но с опциональными условиями) выполняется за O(n), где n – количество строк
![image](https://user-images.githubusercontent.com/79102850/204132152-d132d54a-a20e-4081-9ef0-29f69f5cd6b2.png)

Операция объединения (1 таблица с увеличивающимся числом записей на 1000, вторая с константным числом записей) выполняется за O(n), где n – количество строк в первой таблице
![image](https://user-images.githubusercontent.com/79102850/212277047-1650f9a6-f8c8-40bf-9337-4e810c1bf4d2.png)

Операция объединения (У обоих таблиц развер равный, увеличивается на 1000) выполняется не более чем за O(n*n), где n – количество строк в каждой из таблиц, т.к. используется алгоритм Nested Loop Join. На данных количествах данных тренд приближается к линейному.
![image](https://user-images.githubusercontent.com/79102850/212286502-693b1018-eeb1-4822-83a1-c3f7f38e5800.png)

Операции обновления и удаления элемента данных выполняются не более чем за O(n*m) > t -> O(n+m), где n – количество представленных элементов данных обрабатываемого вида, m – количество фактически затронутых элементов данных

![image](https://user-images.githubusercontent.com/79102850/204132177-651c3b3c-052a-411d-8a94-9ca4d6022524.png)

![image](https://user-images.githubusercontent.com/79102850/204132185-6e87e3e9-fb41-4ec7-b87b-208257bf197c.png)

Запуск на Windows и *NIX

0. Установить утилиту make
1. Перейти в корень проекта
2. Выбрать нужный Makefile. Если это Windows, убрать расширение в названии файла. Удалить лишний файл.
3. Выполнить команды:
```
make //сборка модуля
make exec //запуск
make clean //очистка 
```

#### 4. Выводы

В результате выполнения лабораторной работы был разработан модуль, реализующий хранение в одном файле данные в виде строк в таблицах, примем объем данных может достигать объема от 10GB, имеющий вид реляционной БД. Модуль поддерживает операции Create, Insert, Delete, Update, Join.
Были разработаны тесты, которые доказали, что для реализации операций были выбраны правильные алгоритмы, т.к. алгоритмическая сложность совпала с ожидаемой.
Модуль может работать под управлением ОС семейств Windows и *NIX.




