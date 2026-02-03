//     Keagan Anderson
//        MagBase
//       02/03/2026
//
//     Record management functions for reading/writing/modifying table records

#include "records.h"
#include "schema.h"
#include "buffer.h"
#include "globals.h"
#include <stdlib.h>
#include <string.h>

Record *createRecord(uint16_t table_id, uint16_t field_count) {
    Record *record = malloc(sizeof(Record));
    if (!record) {
        return NULL;
    }

    record->record_id = 0;
    record->table_id = table_id;
    record->field_count = field_count;
    record->fields = malloc(field_count * sizeof(RecordField));

    if (!record->fields) {
        free(record);
        return NULL;
    }

    // Initialize all fields as NULL
    for (uint16_t i = 0; i < field_count; i++) {
        record->fields[i].is_null = 1;
        record->fields[i].type = COL_INT;
        memset(&record->fields[i].value, 0, sizeof(record->fields[i].value));
    }

    return record;
}

void freeRecord(Record *record) {
    if (record) {
        if (record->fields) {
            free(record->fields);
        }
        free(record);
    }
}

// Serialize a record into a buffer
static void serializeRecord(uint8_t *buffer, Record *record) {
    uint8_t *ptr = buffer;

    // Write header
    memcpy(ptr, &record->record_id, sizeof(uint64_t));
    ptr += sizeof(uint64_t);

    memcpy(ptr, &record->table_id, sizeof(uint16_t));
    ptr += sizeof(uint16_t);

    memcpy(ptr, &record->field_count, sizeof(uint16_t));
    ptr += sizeof(uint16_t);

    // Write fields
    for (uint16_t i = 0; i < record->field_count; i++) {
        // Write type and null flag
        memcpy(ptr, &record->fields[i].type, sizeof(uint8_t));
        ptr += sizeof(uint8_t);

        memcpy(ptr, &record->fields[i].is_null, sizeof(uint8_t));
        ptr += sizeof(uint8_t);

        // Write value
        if (!record->fields[i].is_null) {
            switch (record->fields[i].type) {
                case COL_INT:
                    memcpy(ptr, &record->fields[i].value.int_val, sizeof(int32_t));
                    ptr += sizeof(int32_t);
                    break;
                case COL_BOOL:
                    memcpy(ptr, &record->fields[i].value.bool_val, sizeof(uint8_t));
                    ptr += sizeof(uint8_t);
                    break;
                case COL_TEXT: {
                    uint16_t text_len = (uint16_t)strlen(record->fields[i].value.text_val);
                    memcpy(ptr, &text_len, sizeof(uint16_t));
                    ptr += sizeof(uint16_t);
                    memcpy(ptr, record->fields[i].value.text_val, text_len);
                    ptr += text_len;
                    break;
                }
            }
        }
    }
}

// Deserialize a record from a buffer
// Returns the number of bytes consumed
static size_t deserializeRecord(uint8_t *buffer, Record *record) {
    uint8_t *ptr = buffer;

    // Read header
    memcpy(&record->record_id, ptr, sizeof(uint64_t));
    ptr += sizeof(uint64_t);

    memcpy(&record->table_id, ptr, sizeof(uint16_t));
    ptr += sizeof(uint16_t);

    memcpy(&record->field_count, ptr, sizeof(uint16_t));
    ptr += sizeof(uint16_t);

    // Validate field count
    if (record->field_count > MAX_COLUMNS) {
        record->field_count = 0;
        return (size_t)(ptr - buffer);
    }

    // Read fields
    for (uint16_t i = 0; i < record->field_count; i++) {
        memcpy(&record->fields[i].type, ptr, sizeof(uint8_t));
        ptr += sizeof(uint8_t);

        memcpy(&record->fields[i].is_null, ptr, sizeof(uint8_t));
        ptr += sizeof(uint8_t);

        if (!record->fields[i].is_null) {
            switch (record->fields[i].type) {
                case COL_INT:
                    memcpy(&record->fields[i].value.int_val, ptr, sizeof(int32_t));
                    ptr += sizeof(int32_t);
                    break;
                case COL_BOOL:
                    memcpy(&record->fields[i].value.bool_val, ptr, sizeof(uint8_t));
                    ptr += sizeof(uint8_t);
                    break;
                case COL_TEXT: {
                    uint16_t text_len = 0;
                    memcpy(&text_len, ptr, sizeof(uint16_t));
                    ptr += sizeof(uint16_t);

                    // Validate text length
                    if (text_len > MAX_RECORD_VALUE_SIZE) {
                        text_len = MAX_RECORD_VALUE_SIZE - 1;
                    }

                    memcpy(record->fields[i].value.text_val, ptr, text_len);
                    record->fields[i].value.text_val[text_len] = '\0';
                    ptr += text_len;
                    break;
                }
            }
        }
    }

    return (size_t)(ptr - buffer);
}

// Calculate the serialized size of a record
static size_t getRecordSize(Record *record) {
    size_t size = sizeof(uint64_t) + sizeof(uint16_t) + sizeof(uint16_t);  // header

    for (uint16_t i = 0; i < record->field_count; i++) {
        size += sizeof(uint8_t) + sizeof(uint8_t);  // type + is_null

        if (!record->fields[i].is_null) {
            switch (record->fields[i].type) {
                case COL_INT:
                    size += sizeof(int32_t);
                    break;
                case COL_BOOL:
                    size += sizeof(uint8_t);
                    break;
                case COL_TEXT:
                    size += sizeof(uint16_t) + strlen(record->fields[i].value.text_val);
                    break;
            }
        }
    }

    return size;
}

uint64_t insertRecord(MagBase *db, Record *record) {
    if (!db || !record) {
        return 0;
    }

    TableSchemaRecord *schema = readTableSchema(db, record->table_id);
    if (!schema) {
        return 0;
    }

    // Generate record_id from schema's next_record_id (only if not already set)
    if (record->record_id == 0) {
        record->record_id = schema->next_record_id;
        schema->next_record_id++;
    }

    size_t record_size = getRecordSize(record);

    // Find or allocate page for records
    uint64_t page_num = schema->root_page;
    if (page_num == 0) {
        // Allocate first data page
        page_num = db->header->page_count++;
        schema->root_page = page_num;
        // Update the schema with the new root_page
        if (updateTableSchema(db, schema) != 0) {
            fprintf(stderr, "[ERROR] Failed to update table schema with root_page\n");
            free(schema);
            return 0;
        }
    }

    char *page_buffer = readPageFromBuffer(db->buffer_pool, page_num, db->file_pointer, db->page_size);
    if (!page_buffer) {
        free(schema);
        return 0;
    }

    PageHeader *page_header = (PageHeader *)page_buffer;
    
    // Initialize header if this is a new/empty page
    if (page_header->free_space_offset == 0) {
        page_header->slot_count = 0;
        page_header->free_space_offset = sizeof(PageHeader);
        page_header->next_page = 0;
    }
    
    uint16_t available_space = db->page_size - page_header->free_space_offset;

    // Check if record fits
    if (available_space < record_size + sizeof(uint16_t)) {
        // Allocate new page
        uint64_t new_page_num = db->header->page_count++;
        page_header->next_page = new_page_num;
        markPageDirty(db->buffer_pool, page_num);

        // Initialize new page
        page_buffer = readPageFromBuffer(db->buffer_pool, new_page_num, db->file_pointer, db->page_size);
        if (!page_buffer) {
            free(schema);
            return 0;
        }

        page_header = (PageHeader *)page_buffer;
        page_header->slot_count = 0;
        page_header->free_space_offset = sizeof(PageHeader);
        page_header->next_page = 0;
        page_num = new_page_num;
    }

    // Write record
    uint8_t *write_ptr = (uint8_t *)page_buffer + page_header->free_space_offset;
    serializeRecord(write_ptr, record);

    // Update page header
    page_header->slot_count++;
    page_header->free_space_offset += (uint16_t)record_size;

    markPageDirty(db->buffer_pool, page_num);

    // Persist updated next_record_id to schema
    updateTableSchema(db, schema);
    free(schema);

    return record->record_id;
}

Record *readRecord(MagBase *db, uint16_t table_id, uint64_t record_id) {
    if (!db || table_id == 0 || record_id == 0) {
        return NULL;
    }

    TableSchemaRecord *schema = readTableSchema(db, table_id);
    if (!schema) {
        return NULL;
    }

    uint64_t page_num = schema->root_page;

    while (page_num != 0) {
        char *page_buffer = readPageFromBuffer(db->buffer_pool, page_num, db->file_pointer, db->page_size);
        if (!page_buffer) {
            free(schema);
            return NULL;
        }

        PageHeader *page_header = (PageHeader *)page_buffer;
        uint8_t *record_ptr = (uint8_t *)page_buffer + sizeof(PageHeader);

        // Search through records in this page
        for (uint16_t i = 0; i < page_header->slot_count; i++) {
            Record *record = createRecord(table_id, schema->column_count);
            if (!record) {
                free(schema);
                return NULL;
            }

            deserializeRecord(record_ptr, record);

            if (record->record_id == record_id) {
                free(schema);
                return record;
            }

            size_t consumed = getRecordSize(record);
            freeRecord(record);
            record_ptr += consumed;
        }

        page_num = page_header->next_page;
    }

    free(schema);
    return NULL;
}

int updateRecord(MagBase *db, Record *record) {
    if (!db || !record || record->record_id == 0 || record->table_id == 0) {
        return -1;
    }

    TableSchemaRecord *schema = readTableSchema(db, record->table_id);
    if (!schema) {
        return -1;
    }

    uint64_t page_num = schema->root_page;

    while (page_num != 0) {
        char *page_buffer = readPageFromBuffer(db->buffer_pool, page_num, db->file_pointer, db->page_size);
        if (!page_buffer) {
            free(schema);
            return -1;
        }

        PageHeader *page_header = (PageHeader *)page_buffer;
        uint8_t *record_ptr = (uint8_t *)page_buffer + sizeof(PageHeader);

        // Find the record to update
        for (uint16_t i = 0; i < page_header->slot_count; i++) {
            Record temp_record = {0};
            temp_record.fields = malloc(schema->column_count * sizeof(RecordField));
            if (!temp_record.fields) {
                free(schema);
                return -1;
            }
            temp_record.field_count = schema->column_count;

            deserializeRecord(record_ptr, &temp_record);

            if (temp_record.record_id == record->record_id) {
                // Found it - only allow updates if new record is same size or smaller
                size_t old_size = getRecordSize(&temp_record);
                size_t new_size = getRecordSize(record);
                
                if (new_size <= old_size) {
                    // Safe to update in place
                    serializeRecord(record_ptr, record);
                    markPageDirty(db->buffer_pool, page_num);
                    free(temp_record.fields);
                    free(schema);
                    return 0;
                } else {
                    // Record is getting larger - cannot update in place
                    free(temp_record.fields);
                    free(schema);
                    return -1;
                }
            }

            size_t consumed = getRecordSize(&temp_record);
            free(temp_record.fields);
            record_ptr += consumed;
        }

        page_num = page_header->next_page;
    }

    free(schema);
    return -1;  // Record not found
}

int deleteRecord(MagBase *db, uint16_t table_id, uint64_t record_id) {
    if (!db || table_id == 0 || record_id == 0) {
        return -1;
    }

    TableSchemaRecord *schema = readTableSchema(db, table_id);
    if (!schema) {
        return -1;
    }

    uint64_t page_num = schema->root_page;

    while (page_num != 0) {
        char *page_buffer = readPageFromBuffer(db->buffer_pool, page_num, db->file_pointer, db->page_size);
        if (!page_buffer) {
            free(schema);
            return -1;
        }

        PageHeader *page_header = (PageHeader *)page_buffer;
        uint8_t *record_ptr = (uint8_t *)page_buffer + sizeof(PageHeader);

        // Find and remove the record
        for (uint16_t i = 0; i < page_header->slot_count; i++) {
            Record temp_record = {0};
            temp_record.fields = malloc(schema->column_count * sizeof(RecordField));
            if (!temp_record.fields) {
                free(schema);
                return -1;
            }
            temp_record.field_count = schema->column_count;

            deserializeRecord(record_ptr, &temp_record);

            if (temp_record.record_id == record_id) {
                // Found it - shift remaining records back
                size_t consumed = getRecordSize(&temp_record);
                uint8_t *next_record_ptr = record_ptr + consumed;
                size_t remaining = page_header->free_space_offset - (size_t)(next_record_ptr - (uint8_t *)page_buffer);

                memmove(record_ptr, next_record_ptr, remaining);

                page_header->slot_count--;
                page_header->free_space_offset -= (uint16_t)consumed;

                markPageDirty(db->buffer_pool, page_num);
                free(temp_record.fields);
                free(schema);
                return 0;
            }

            free(temp_record.fields);
            record_ptr += getRecordSize(&temp_record);
        }

        page_num = page_header->next_page;
    }

    free(schema);
    return -1;
}

Record **readAllRecords(MagBase *db, uint16_t table_id, uint64_t *num_records) {
    if (!db || table_id == 0 || !num_records) {
        return NULL;
    }

    TableSchemaRecord *schema = readTableSchema(db, table_id);
    if (!schema) {
        return NULL;
    }

    // First pass: count records
    uint64_t total_records = 0;
    uint64_t page_num = schema->root_page;

    while (page_num != 0) {
        char *page_buffer = readPageFromBuffer(db->buffer_pool, page_num, db->file_pointer, db->page_size);
        if (!page_buffer) {
            free(schema);
            return NULL;
        }

        PageHeader *page_header = (PageHeader *)page_buffer;
        total_records += page_header->slot_count;
        page_num = page_header->next_page;
    }

    if (total_records == 0) {
        *num_records = 0;
        free(schema);
        return NULL;
    }

    // Allocate array
    Record **records = malloc(total_records * sizeof(Record *));
    if (!records) {
        free(schema);
        return NULL;
    }

    // Second pass: read all records
    uint64_t record_index = 0;
    page_num = schema->root_page;

    while (page_num != 0) {
        char *page_buffer = readPageFromBuffer(db->buffer_pool, page_num, db->file_pointer, db->page_size);
        if (!page_buffer) {
            for (uint64_t i = 0; i < record_index; i++) {
                freeRecord(records[i]);
            }
            free(records);
            free(schema);
            return NULL;
        }

        PageHeader *page_header = (PageHeader *)page_buffer;
        uint8_t *record_ptr = (uint8_t *)page_buffer + sizeof(PageHeader);

        for (uint16_t i = 0; i < page_header->slot_count; i++) {
            Record *record = createRecord(table_id, schema->column_count);
            if (!record) {
                for (uint64_t j = 0; j < record_index; j++) {
                    freeRecord(records[j]);
                }
                free(records);
                free(schema);
                return NULL;
            }

            deserializeRecord(record_ptr, record);
            records[record_index++] = record;
            record_ptr += getRecordSize(record);
        }

        page_num = page_header->next_page;
    }

    *num_records = total_records;
    free(schema);
    return records;
}
