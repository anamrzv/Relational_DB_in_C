#include <inttypes.h>
#include "../include/file.h"

enum open_status open_file(FILE **in, const char *const filename, const char *const mode) {
    *in = fopen(filename, mode);
    if (*in == NULL) return OPEN_ERROR;
    else return OPEN_OK;
}

enum close_status close_file(FILE *in) {
    if (fclose(in) == 0)return CLOSE_OK;
    else return CLOSE_ERROR;
}

enum write_status write_header_to_tech_page(FILE *file, struct page* tech_page, struct page* table_page) {
    if (table_page == NULL) {
        fseek(file, 0, SEEK_SET);
        if (fwrite(tech_page->page_header->database_header, sizeof(struct database_header), 1, file) == 1) {
            tech_page->page_header->free_space_cursor += sizeof(struct database_header);
            tech_page->page_header->free_bytes -= sizeof(struct database_header);
            return WRITE_OK;
        }
        else return WRITE_ERROR;
    } else {
        if (!enough_free_space(table_page, sizeof(struct table_header))) {
            struct page* new_page = add_page_back_to_db_header_list(tech_page->page_header->database_header);
            tech_page = new_page;
            if (overwrite_dh_after_change(file, tech_page->page_header->database_header) != WRITE_OK) return WRITE_ERROR;
        }
        fseek(file, (tech_page->page_header->page_number_general-1)*DEFAULT_PAGE_SIZE_B + tech_page->page_header->free_space_cursor, SEEK_SET);
        if (fwrite(table_page->page_header->table_header, sizeof(struct table_header), 1, file) == 1) {
            tech_page->page_header->free_space_cursor += sizeof(struct table_header);
            tech_page->page_header->free_bytes -= sizeof(struct table_header);
            return WRITE_OK;
        }
        else return WRITE_ERROR;
    }
}

enum write_status overwrite_after_table_delete(FILE *file, struct table_header* deleted_table_header, struct table_header* left_neighnour_header, struct database_header* db_header) {
    if (left_neighnour_header == NULL) {
        db_header->next = deleted_table_header->next;
        if (db_header->next == NULL) db_header->last_table_header = NULL;
        fseek(file, sizeof(struct database_header) + (deleted_table_header->number_in_tech_page-1)*sizeof(struct table_header), SEEK_SET);
        if (fwrite(deleted_table_header, sizeof(struct table_header), 1, file) != 1) return WRITE_ERROR;
    } else {
        left_neighnour_header->next = deleted_table_header->next;
        if (left_neighnour_header->next = NULL) db_header->last_table_header = left_neighnour_header;
        fseek(file, sizeof(struct database_header) + (left_neighnour_header->number_in_tech_page-1)*sizeof(struct table_header), SEEK_SET);
        if (fwrite(left_neighnour_header, sizeof(struct table_header), 1, file) == 1) {
            if (fwrite(deleted_table_header, sizeof(struct table_header), 1, file) != 1) return WRITE_ERROR;
        } else return WRITE_ERROR;
    }
    fseek(file, 0, SEEK_SET);
    if (fwrite(db_header, sizeof(struct database_header), 1, file) == 1) return WRITE_OK;
    else return WRITE_ERROR;
}

enum write_status overwrite_th_after_change(FILE *file, struct table_header* changed_table_header) {
    fseek(file, sizeof(struct database_header) + (changed_table_header->number_in_tech_page-1)*sizeof(struct table_header), SEEK_SET);
    if (fwrite(changed_table_header, sizeof(struct table_header), 1, file) == 1) return WRITE_OK;
    else return WRITE_ERROR;
}

enum write_status overwrite_dh_after_change(FILE *file, struct database_header* changed_db_header) {
    fseek(file, 0, SEEK_SET);
    if (fwrite(changed_db_header, sizeof(struct table_header), 1, file) == 1) return WRITE_OK;
    else return WRITE_ERROR;
}

enum write_status write_table_page_first_time(FILE *file, struct page* page_to_write) {
    fseek(file, (page_to_write->page_header->page_number_general-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
    if (fwrite(page_to_write->page_header, sizeof(struct page_header), 1, file) == 1) {
        page_to_write->page_header->free_bytes -= sizeof(struct page_header);
        page_to_write->page_header->free_space_cursor += sizeof(struct page_header);
        return WRITE_OK;
    }
    return WRITE_ERROR;
}

enum write_status write_row_to_page(FILE *file, struct page* page_to_write, struct row* row) {
    uint32_t row_len = page_to_write->page_header->table_header->table->table_schema->row_length;
    uint32_t sum_volume = sizeof(struct row_header) + row_len;
    if (!enough_free_space(page_to_write, sum_volume)) {
            struct page* new_page = add_page_back_to_table_header_list(page_to_write->page_header->table_header);
            page_to_write = new_page;
            if (overwrite_dh_after_change(file, page_to_write->page_header->database_header) != WRITE_OK) return WRITE_ERROR;
            if (overwrite_th_after_change(file, page_to_write->page_header->table_header) != WRITE_OK) return WRITE_ERROR;
    }
    fseek(file, (page_to_write->page_header->page_number_general-1)*DEFAULT_PAGE_SIZE_B + page_to_write->page_header->free_space_cursor, SEEK_SET);
    if (fwrite(row->row_header, sizeof(struct row_header), 1, file) == 1) { //вроде не нужен отступ
        if (fwrite(row->content, row_len, 1, file) == 1) {
            page_to_write->page_header->free_bytes -= sizeof(struct row_header) + row_len;
            page_to_write->page_header->free_space_cursor += sizeof(struct row_header) + row_len;
            return WRITE_OK;
        }
    }
    return WRITE_ERROR;
}

enum read_status read_database_header(FILE *file, struct database_header* db_header) {
   fseek(file, 0, SEEK_SET);
   if (fread(db_header, sizeof(struct database_header), 1, file) == 1) return READ_OK;
   else return READ_ERROR;
}