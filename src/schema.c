//     Keagan Anderson
//        MagBase
//       02/03/2026
//
//     Schema management functions for reading/writing table schemas

#include "schema.h"
#include "buffer.h"
#include "globals.h"
#include <stdlib.h>
#include <string.h>

// Calculate the serialized size of a TableSchemaRecord
static size_t getSchemaRecordSize(TableSchemaRecord *schema) {
    // Fixed fields: table_id (2) + column_count (2) + root_page (4) + name_len (2)
    // Variable: table_name (up to MAX_TABLE_NAME) + columns array (MAX_COLUMNS * size)
    size_t size = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t);
    size += schema->name_len;  // actual name length
    size += schema->column_count * sizeof(SchemaColumn);
    return size;
}

// Serialize a TableSchemaRecord into a buffer
static void serializeSchemaRecord(uint8_t *buffer, TableSchemaRecord *schema) {
    uint8_t *ptr = buffer;

    // Write fixed fields
    memcpy(ptr, &schema->table_id, sizeof(uint16_t));
    ptr += sizeof(uint16_t);

    memcpy(ptr, &schema->column_count, sizeof(uint16_t));
    ptr += sizeof(uint16_t);

    memcpy(ptr, &schema->root_page, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    memcpy(ptr, &schema->name_len, sizeof(uint16_t));
    ptr += sizeof(uint16_t);

    // Write table name
    memcpy(ptr, schema->table_name, schema->name_len);
    ptr += schema->name_len;

    // Write columns
    for (int i = 0; i < schema->column_count; i++) {
        memcpy(ptr, &schema->columns[i].type, sizeof(uint8_t));
        ptr += sizeof(uint8_t);

        memcpy(ptr, &schema->columns[i].nullable, sizeof(uint8_t));
        ptr += sizeof(uint8_t);

        memcpy(ptr, &schema->columns[i].name_len, sizeof(uint16_t));
        ptr += sizeof(uint16_t);

        memcpy(ptr, schema->columns[i].name, schema->columns[i].name_len);
        ptr += schema->columns[i].name_len;
    }
}

// Deserialize a TableSchemaRecord from a buffer
// Returns the number of bytes consumed
static size_t deserializeSchemaRecord(uint8_t *buffer, TableSchemaRecord *schema) {
    uint8_t *ptr = buffer;

    // Read fixed fields
    memcpy(&schema->table_id, ptr, sizeof(uint16_t));
    ptr += sizeof(uint16_t);

    memcpy(&schema->column_count, ptr, sizeof(uint16_t));
    ptr += sizeof(uint16_t);

    memcpy(&schema->root_page, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    memcpy(&schema->name_len, ptr, sizeof(uint16_t));
    ptr += sizeof(uint16_t);

    // Read table name
    memcpy(schema->table_name, ptr, schema->name_len);
    schema->table_name[schema->name_len] = '\0';
    ptr += schema->name_len;

    // Read columns
    schema->column_count = (schema->column_count > MAX_COLUMNS) ? MAX_COLUMNS : schema->column_count;
    for (int i = 0; i < schema->column_count; i++) {
        memcpy(&schema->columns[i].type, ptr, sizeof(uint8_t));
        ptr += sizeof(uint8_t);

        memcpy(&schema->columns[i].nullable, ptr, sizeof(uint8_t));
        ptr += sizeof(uint8_t);

        memcpy(&schema->columns[i].name_len, ptr, sizeof(uint16_t));
        ptr += sizeof(uint16_t);

        memcpy(schema->columns[i].name, ptr, schema->columns[i].name_len);
        schema->columns[i].name[schema->columns[i].name_len] = '\0';
        ptr += schema->columns[i].name_len;
    }

    return (size_t)(ptr - buffer);
}

int writeTableSchema(MagBase *db, TableSchemaRecord *schema) {
    if (!db || !schema) {
        return -1;
    }

    size_t record_size = getSchemaRecordSize(schema);

    // Find available space in schema pages, starting from schema_root
    uint64_t page_num = db->header->schema_root;
    char *page_buffer = NULL;

    while (1) {
        // Read the schema page
        page_buffer = readPageFromBuffer(db->buffer_pool, page_num, db->file_pointer, db->page_size);
        if (!page_buffer) {
            return -1;
        }

        SchemaPageHeader *schema_header = (SchemaPageHeader *)page_buffer;
        uint16_t available_space = db->page_size - schema_header->free_space_offset;

        // Check if record fits in this page
        if (available_space >= record_size + sizeof(uint16_t)) {  // +2 for offset entry
            // Write the record
            uint8_t *write_ptr = (uint8_t *)page_buffer + schema_header->free_space_offset;
            serializeSchemaRecord(write_ptr, schema);

            // Update schema header
            schema_header->table_count++;
            schema_header->free_space_offset += (uint16_t)record_size;

            // Mark page as dirty
            markPageDirty(db->buffer_pool, page_num);

            // Assign table_id if not already set
            if (schema->table_id == 0) {
                schema->table_id = schema_header->table_count;
            }

            return (int)schema->table_id;
        }

        // Move to next schema page
        if (schema_header->next_schema_page == 0) {
            // Allocate a new schema page
            uint64_t new_page_num = db->header->page_count++;
            schema_header->next_schema_page = new_page_num;
            markPageDirty(db->buffer_pool, page_num);

            // Initialize new schema page
            page_buffer = readPageFromBuffer(db->buffer_pool, new_page_num, db->file_pointer, db->page_size);
            if (!page_buffer) {
                return -1;
            }

            SchemaPageHeader *new_header = (SchemaPageHeader *)page_buffer;
            new_header->table_count = 0;
            new_header->free_space_offset = sizeof(SchemaPageHeader);
            new_header->next_schema_page = 0;
            markPageDirty(db->buffer_pool, new_page_num);

            page_num = new_page_num;
            continue;
        }

        page_num = schema_header->next_schema_page;
    }

    return -1;
}

TableSchemaRecord *readTableSchema(MagBase *db, uint16_t table_id) {
    if (!db || table_id == 0) {
        return NULL;
    }

    uint64_t page_num = db->header->schema_root;

    while (page_num != 0) {
        // Read the schema page
        char *page_buffer = readPageFromBuffer(db->buffer_pool, page_num, db->file_pointer, db->page_size);
        if (!page_buffer) {
            return NULL;
        }

        SchemaPageHeader *schema_header = (SchemaPageHeader *)page_buffer;
        uint8_t *record_ptr = (uint8_t *)page_buffer + sizeof(SchemaPageHeader);

        // Search through records in this page
        for (uint16_t i = 0; i < schema_header->table_count; i++) {
            TableSchemaRecord *schema = malloc(sizeof(TableSchemaRecord));
            if (!schema) {
                return NULL;
            }

            size_t consumed = deserializeSchemaRecord(record_ptr, schema);

            if (schema->table_id == table_id) {
                return schema;
            }

            free(schema);
            record_ptr += consumed;
        }

        // Move to next schema page
        page_num = schema_header->next_schema_page;
    }

    return NULL;
}

TableSchemaRecord **readAllTableSchemas(MagBase *db, uint16_t *num_tables) {
    if (!db || !num_tables) {
        return NULL;
    }

    // First pass: count total tables
    uint16_t total_tables = 0;
    uint64_t page_num = db->header->schema_root;

    while (page_num != 0) {
        char *page_buffer = readPageFromBuffer(db->buffer_pool, page_num, db->file_pointer, db->page_size);
        if (!page_buffer) {
            return NULL;
        }

        SchemaPageHeader *schema_header = (SchemaPageHeader *)page_buffer;
        total_tables += schema_header->table_count;
        page_num = schema_header->next_schema_page;
    }

    if (total_tables == 0) {
        *num_tables = 0;
        return NULL;
    }

    // Allocate array
    TableSchemaRecord **schemas = malloc(total_tables * sizeof(TableSchemaRecord *));
    if (!schemas) {
        return NULL;
    }

    // Second pass: read all schemas
    uint16_t schema_index = 0;
    page_num = db->header->schema_root;

    while (page_num != 0) {
        char *page_buffer = readPageFromBuffer(db->buffer_pool, page_num, db->file_pointer, db->page_size);
        if (!page_buffer) {
            free(schemas);
            return NULL;
        }

        SchemaPageHeader *schema_header = (SchemaPageHeader *)page_buffer;
        uint8_t *record_ptr = (uint8_t *)page_buffer + sizeof(SchemaPageHeader);

        for (uint16_t i = 0; i < schema_header->table_count; i++) {
            TableSchemaRecord *schema = malloc(sizeof(TableSchemaRecord));
            if (!schema) {
                // Free previously allocated schemas on error
                for (uint16_t j = 0; j < schema_index; j++) {
                    free(schemas[j]);
                }
                free(schemas);
                return NULL;
            }

            deserializeSchemaRecord(record_ptr, schema);
            schemas[schema_index++] = schema;
            record_ptr += getSchemaRecordSize(schema);
        }

        page_num = schema_header->next_schema_page;
    }

    *num_tables = total_tables;
    return schemas;
}

int deleteTableSchema(MagBase *db, uint16_t table_id) {
    if (!db || table_id == 0) {
        return -1;
    }

    uint64_t page_num = db->header->schema_root;
    TableSchemaRecord temp_schema;

    while (page_num != 0) {
        char *page_buffer = readPageFromBuffer(db->buffer_pool, page_num, db->file_pointer, db->page_size);
        if (!page_buffer) {
            return -1;
        }

        SchemaPageHeader *schema_header = (SchemaPageHeader *)page_buffer;
        uint8_t *record_ptr = (uint8_t *)page_buffer + sizeof(SchemaPageHeader);
        uint8_t *prev_record_ptr = record_ptr;

        // Find and remove the schema
        for (uint16_t i = 0; i < schema_header->table_count; i++) {
            size_t consumed = deserializeSchemaRecord(record_ptr, &temp_schema);

            if (temp_schema.table_id == table_id) {
                // Found it - shift remaining records back
                uint8_t *next_record_ptr = record_ptr + consumed;
                size_t remaining = schema_header->free_space_offset - (size_t)(next_record_ptr - (uint8_t *)page_buffer);

                memmove(record_ptr, next_record_ptr, remaining);

                schema_header->table_count--;
                schema_header->free_space_offset -= (uint16_t)consumed;

                markPageDirty(db->buffer_pool, page_num);
                return 0;
            }

            prev_record_ptr = record_ptr;
            record_ptr += consumed;
        }

        page_num = schema_header->next_schema_page;
    }

    return -1;  // Table not found
}

int addColumnToTable(MagBase *db, uint16_t table_id, SchemaColumn *column) {
    if (!db || table_id == 0 || !column) {
        return -1;
    }

    TableSchemaRecord *schema = readTableSchema(db, table_id);
    if (!schema) {
        return -1;
    }

    // Check if we can add more columns
    if (schema->column_count >= MAX_COLUMNS) {
        free(schema);
        return -1;
    }

    // Add the column
    memcpy(&schema->columns[schema->column_count], column, sizeof(SchemaColumn));
    schema->column_count++;

    // Delete old schema and write updated one
    deleteTableSchema(db, table_id);
    int result = writeTableSchema(db, schema);

    free(schema);
    return (result > 0) ? 0 : -1;
}

int removeColumnFromTable(MagBase *db, uint16_t table_id, uint16_t column_index) {
    if (!db || table_id == 0) {
        return -1;
    }

    TableSchemaRecord *schema = readTableSchema(db, table_id);
    if (!schema || column_index >= schema->column_count) {
        if (schema) free(schema);
        return -1;
    }

    // Shift columns back
    for (uint16_t i = column_index; i < schema->column_count - 1; i++) {
        memcpy(&schema->columns[i], &schema->columns[i + 1], sizeof(SchemaColumn));
    }
    schema->column_count--;

    // Delete old schema and write updated one
    deleteTableSchema(db, table_id);
    int result = writeTableSchema(db, schema);

    free(schema);
    return (result > 0) ? 0 : -1;
}

int updateTableColumn(MagBase *db, uint16_t table_id, uint16_t column_index, SchemaColumn *new_column) {
    if (!db || table_id == 0 || !new_column) {
        return -1;
    }

    TableSchemaRecord *schema = readTableSchema(db, table_id);
    if (!schema || column_index >= schema->column_count) {
        if (schema) free(schema);
        return -1;
    }

    // Update the column
    memcpy(&schema->columns[column_index], new_column, sizeof(SchemaColumn));

    // Delete old schema and write updated one
    deleteTableSchema(db, table_id);
    int result = writeTableSchema(db, schema);

    free(schema);
    return (result > 0) ? 0 : -1;
}
