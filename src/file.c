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

enum write_status write_db_to_file(FILE *file, struct database_header* db_header, struct page_header* tech_page_header) {
    fseek(file, 0, SEEK_SET);
    if (fwrite(db_header, sizeof(struct database_header), 1, file) == 1) {
        tech_page_header->free_space_cursor += sizeof(struct database_header)+sizeof(struct page_header);
        tech_page_header->free_bytes -= sizeof(struct database_header)+sizeof(struct page_header);
        if (fwrite(tech_page_header, sizeof(struct page_header), 1, file) == 1) return WRITE_OK;
    }
    return WRITE_ERROR;
}

enum write_status write_header_to_tech_page(FILE *file, struct database_header* db_header, struct page_header* table_page_header) {
    fseek(file, sizeof(struct database_header), SEEK_SET);
    struct page_header* tech_page_header = malloc(sizeof(struct page_header));
    fread(tech_page_header, sizeof(struct page_header), 1, file);
    if (!enough_free_space(tech_page_header, sizeof(struct table_header))) {
        struct page_header* new_page_header = add_tech_page(db_header);
        tech_page_header = new_page_header;
        if (overwrite_dh_after_change(file, db_header) != WRITE_OK) return WRITE_ERROR;
    }
    fseek(file, tech_page_header->free_space_cursor, SEEK_SET);
    if (fwrite(table_page_header, sizeof(struct table_header), 1, file) == 1) {
        tech_page_header->free_space_cursor += sizeof(struct table_header);
        tech_page_header->free_bytes -= sizeof(struct table_header);
        return WRITE_OK;
    }
    else return WRITE_ERROR;    
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

enum write_status write_row_to_page(FILE *file, uint32_t page_to_write_num, struct row* row) {
    uint32_t row_len = row->table->table_header->schema.row_length;
    uint32_t sum_volume = sizeof(struct row_header) + row_len;
    fseek(file, (page_to_write_num-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
    struct page_header* ph_to_write = malloc(sizeof(struct page_header));
    if (fread(ph_to_write, sizeof(struct page_header), 1, file) == 1) {
        if (!enough_free_space(ph_to_write, sum_volume)) {
            struct page* new_page = add_page_back_to_table_header_list(row->table->table_header);
            if (overwrite_dh_after_change(file, row->table->table_header->db->database_header) != WRITE_OK) return WRITE_ERROR;
            if (overwrite_th_after_change(file, row->table->table_header) != WRITE_OK) return WRITE_ERROR;
            page_to_write_num = new_page->page_header->page_number_general;
            ph_to_write = new_page->page_header;
        }
        fseek(file, (page_to_write_num-1)*DEFAULT_PAGE_SIZE_B + ph_to_write->free_space_cursor, SEEK_SET);
        if (fwrite(row->row_header, sizeof(struct row_header), 1, file) == 1) { //вроде не нужен отступ
            if (fwrite(row->content, row_len, 1, file) == 1) {
                ph_to_write->free_bytes -= sizeof(struct row_header) + row_len;
                ph_to_write->free_space_cursor += sizeof(struct row_header) + row_len;

                fseek(file, (page_to_write_num-1)*DEFAULT_PAGE_SIZE_B, SEEK_SET);
                if (fwrite(ph_to_write, sizeof(struct page_header), 1, file) != 1) return WRITE_ERROR;
                return WRITE_OK;
            }
        }
        return WRITE_ERROR;
    } else return WRITE_ERROR;
    
}

enum read_status read_database_header(FILE *file, struct database_header* db_header) {
   fseek(file, 0, SEEK_SET);
   if (fread(db_header, sizeof(struct database_header), 1, file) == 1) return READ_OK;
   else return READ_ERROR;
}

enum read_status read_table_header(FILE *file, const char *const tablename, struct table_header* read_th, size_t table_count) {
    fseek(file, sizeof(struct database_header), SEEK_SET);
    size_t cnt = 0;
    while (cnt != table_count) {
        if (fread(read_th, sizeof(struct table_header), 1, file) == 1) {
            if (strcmp(tablename, read_th->name) == 0) return READ_OK;
        } else return READ_ERROR;
        cnt++;
    }
    return READ_ERROR;
}