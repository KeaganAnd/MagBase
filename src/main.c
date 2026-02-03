//
//
//
//          MagBase
//       A Maggie Database
//       A toy database
//       Start: 01/08/2026
//
//       ..it's weird to write 2026

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db-init.h"
#include "globals.h"
#include "schema.h"
#include "records.h"
#include "buffer.h"
#include "structs/schemaStruct.h"

Version version = {DB_VERSION_MAJOR, DB_VERSION_MINOR, DB_VERSION_PATCH};

int main(int argc, char *argv[]) {

    if (argc == 1) {
        printf("Welcome page\n");
        return 0;
    }

    // Flag parser
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-v") ||
            !strcmp(argv[i], "-version")) { // strcmp returns 0 if they're equal,
                                            // performs a binary comparison
            printf("Version %d.%d.%d\n", version.major, version.minor, version.patch);

        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-help")) {
            char *helpContent = getHelpContent();
            printf("%s", helpContent);
            free(helpContent);

        } else if (!strcmp(argv[i], "-p")) {
            // Check if files exists if not make it

            char *path = appendFileExt(argv[++i]);

            if (!checkIfFileExists(path)) {
                createDatabase(path);
            }
            printf("Opening MagBase at %s\n", path);

            // Open file then read header, if nothing exists write the header if it does read the
            // data
            FILE *tempFileP = fopen(path, "r");
            if (!tempFileP) {
                fprintf(stderr, "File failed to open while reading header.");
                exit(1);
            }

            Header *header = malloc(sizeof(Header));
            MagBase *magBase;

            size_t length = fread(header, sizeof(Header), 1,
                                  tempFileP); // If the file has no data write the boilerplate
            if (length != 1) {
                if (feof(tempFileP)) {
                    printf("Database is empty, initializing...\n");

                    header = createHeader(header);
                    magBase = createMagBase(header, path, true);

                    int result = writeHeader(magBase);
                    if (result == 0) {
                        printf("Database initialized.\n");
                        fclose(tempFileP);
                    } else {
                        fprintf(stderr, "Database failed to initializing. ABORT\n");
                        fclose(tempFileP);
                        exit(1);
                    }
                }
            } else { // If has data check validity and read data

                if (strcmp(header->magic, MAGIC) != 0) {
                    fprintf(stderr, "This is not a valid MagDB, ensure you are opening the correct "
                                    "file or it may be corrupted.");
                    fclose(tempFileP);
                    exit(1);
                }
                fclose(tempFileP); // Runtime pointer gets made for the struct below

                magBase = createMagBase(header, path, false);
                printf("%d", magBase->header->version.major);
                if (magBase->header->version.major != version.major) {

                    if (magBase->header->version.major >= version.major) {
                        printf("WARNING: Database file is ahead of app\n");
                    } else {
                        printf("WARNING: Database file is behind app\n");
                    }
                    printf("%d.%d.%d vs %d.%d.%d\n", magBase->header->version.major,
                           magBase->header->version.minor, magBase->header->version.patch,
                           version.major, version.minor, version.patch);
                }
            }

            freeDatabase(magBase);
            exit(0);

        } else if (!strcmp(argv[i], "-create-table")) {
            // Create a new table schema
            // Usage: -create-table <db_path> <table_name> <num_columns> [col_name:type:nullable ...]
            if (i + 4 >= argc) {
                fprintf(stderr, "Usage: -create-table <db_path> <table_name> <num_columns> [col_name:type:nullable ...]\n");
                exit(1);
            }

            if (i + 1 >= argc) {
                fprintf(stderr, "Error: database path required\n");
                exit(1);
            }
            char *path = appendFileExt(argv[++i]);
            char *table_name = argv[++i];
            uint16_t num_columns = (uint16_t)atoi(argv[++i]);

            if (num_columns == 0 || num_columns > MAX_COLUMNS) {
                fprintf(stderr, "Invalid number of columns\n");
                exit(1);
            }

            FILE *dbFile = fopen(path, "r+b");
            if (!dbFile) {
                fprintf(stderr, "Failed to open database file\n");
                exit(1);
            }

            Header *header = malloc(sizeof(Header));
            if (fread(header, sizeof(Header), 1, dbFile) != 1) {
                fprintf(stderr, "Failed to read database header\n");
                fclose(dbFile);
                exit(1);
            }

            MagBase *db = createMagBase(header, path, false);
            TableSchemaRecord *schema = malloc(sizeof(TableSchemaRecord));
            memset(schema, 0, sizeof(TableSchemaRecord));

            schema->table_id = 0;  // Will be assigned by writeTableSchema
            schema->column_count = num_columns;
            schema->root_page = 0;  // Will be assigned later
            schema->name_len = (uint16_t)strlen(table_name);
            strncpy(schema->table_name, table_name, MAX_TABLE_NAME - 1);

            // Parse columns
            for (uint16_t col = 0; col < num_columns; col++) {
                if (i + 1 >= argc) {
                    fprintf(stderr, "Not enough column definitions\n");
                    exit(1);
                }
                i++;

                char *col_def = argv[i];
                char *col_name = strtok(col_def, ":");
                char *col_type = strtok(NULL, ":");
                char *col_nullable = strtok(NULL, ":");

                if (!col_name || !col_type) {
                    fprintf(stderr, "Invalid column format. Use: col_name:type:nullable (types: int, text, bool)\n");
                    exit(1);
                }

                schema->columns[col].name_len = (uint16_t)strlen(col_name);
                strncpy(schema->columns[col].name, col_name, MAX_COLUMN_NAME - 1);

                if (!strcmp(col_type, "int")) {
                    schema->columns[col].type = COL_INT;
                } else if (!strcmp(col_type, "text")) {
                    schema->columns[col].type = COL_TEXT;
                } else if (!strcmp(col_type, "bool")) {
                    schema->columns[col].type = COL_BOOL;
                } else {
                    fprintf(stderr, "Unknown column type: %s\n", col_type);
                    exit(1);
                }

                schema->columns[col].nullable = (col_nullable && !strcmp(col_nullable, "1")) ? 1 : 0;
            }

            int table_id = writeTableSchema(db, schema);
            if (table_id > 0) {
                // Flush all dirty pages before writing header
                flushAllDirtyPages(db->buffer_pool, db);
                writeHeader(db);
                printf("Table '%s' created with ID %d\n", table_name, table_id);
            } else {
                fprintf(stderr, "Failed to create table\n");
            }

            free(schema);
            freeDatabase(db);
            exit(0);

        } else if (!strcmp(argv[i], "-list-tables")) {
            // List all tables in the database
            if (i + 1 >= argc) {
                fprintf(stderr, "Usage: -list-tables <db_path>\n");
                exit(1);
            }
            char *path = appendFileExt(argv[++i]);

            FILE *dbFile = fopen(path, "r+b");
            if (!dbFile) {
                fprintf(stderr, "Failed to open database file\n");
                exit(1);
            }

            Header *header = malloc(sizeof(Header));
            if (fread(header, sizeof(Header), 1, dbFile) != 1) {
                fprintf(stderr, "Failed to read database header\n");
                fclose(dbFile);
                exit(1);
            }

            MagBase *db = createMagBase(header, path, false);

            uint16_t num_tables = 0;
            TableSchemaRecord **schemas = readAllTableSchemas(db, &num_tables);

            if (num_tables == 0) {
                printf("No tables found\n");
            } else {
                printf("Tables in database:\n");
                for (uint16_t i = 0; i < num_tables; i++) {
                    printf("  [ID %d] %s (%d columns)\n", schemas[i]->table_id, 
                           schemas[i]->table_name, schemas[i]->column_count);
                    for (uint16_t col = 0; col < schemas[i]->column_count; col++) {
                        const char *type_str = "unknown";
                        switch (schemas[i]->columns[col].type) {
                            case COL_INT:
                                type_str = "int";
                                break;
                            case COL_TEXT:
                                type_str = "text";
                                break;
                            case COL_BOOL:
                                type_str = "bool";
                                break;
                        }
                        printf("    - %s (%s)%s\n", schemas[i]->columns[col].name, type_str,
                               schemas[i]->columns[col].nullable ? " [nullable]" : "");
                    }
                    free(schemas[i]);
                }
                free(schemas);
            }

            freeDatabase(db);
            exit(0);

        } else if (!strcmp(argv[i], "-delete-table")) {
            // Delete a table schema
            // Usage: -delete-table <db_path> <table_id>
            if (i + 2 >= argc) {
                fprintf(stderr, "Usage: -delete-table <db_path> <table_id>\n");
                exit(1);
            }

            if (i + 1 >= argc) {
                fprintf(stderr, "Error: database path required\n");
                exit(1);
            }
            char *path = appendFileExt(argv[++i]);
            uint16_t table_id = (uint16_t)atoi(argv[++i]);

            FILE *dbFile = fopen(path, "r+b");
            if (!dbFile) {
                fprintf(stderr, "Failed to open database file\n");
                exit(1);
            }

            Header *header = malloc(sizeof(Header));
            if (fread(header, sizeof(Header), 1, dbFile) != 1) {
                fprintf(stderr, "Failed to read database header\n");
                fclose(dbFile);
                exit(1);
            }

            MagBase *db = createMagBase(header, path, false);

            int result = deleteTableSchema(db, table_id);
            if (result == 0) {
                flushAllDirtyPages(db->buffer_pool, db);
                writeHeader(db);
                printf("Table with ID %d deleted\n", table_id);
            } else {
                fprintf(stderr, "Failed to delete table\n");
            }

            freeDatabase(db);
            exit(0);

        } else if (!strcmp(argv[i], "-insert-record")) {
            // Insert a new record into a table
            // Usage: -insert-record <db_path> <table_id> [field_value ...]
            if (i + 2 >= argc) {
                fprintf(stderr, "Usage: -insert-record <db_path> <table_id> [field_value ...]\n");
                exit(1);
            }

            if (i + 1 >= argc) {
                fprintf(stderr, "Error: database path required\n");
                exit(1);
            }
            
            // Create path buffer on stack to avoid corrupting argv
            char path_buffer[256];
            strncpy(path_buffer, argv[++i], sizeof(path_buffer) - 5);
            path_buffer[sizeof(path_buffer) - 5] = '\0';
            if (strlen(path_buffer) < 4 || strcmp(&path_buffer[strlen(path_buffer) - 4], ".mab") != 0) {
                strcat(path_buffer, ".mab");
            }
            
            uint16_t table_id = (uint16_t)atoi(argv[++i]);

            FILE *dbFile = fopen(path_buffer, "r+b");
            if (!dbFile) {
                fprintf(stderr, "Failed to open database file\n");
                exit(1);
            }

            Header *header = malloc(sizeof(Header));
            if (fread(header, sizeof(Header), 1, dbFile) != 1) {
                fprintf(stderr, "Failed to read database header\n");
                fclose(dbFile);
                exit(1);
            }

            MagBase *db = createMagBase(header, path_buffer, false);
            TableSchemaRecord *schema = readTableSchema(db, table_id);
            if (!schema) {
                fprintf(stderr, "Table not found\n");
                freeDatabase(db);
                exit(1);
            }

            Record *record = createRecord(table_id, schema->column_count);

            // Parse field values
            for (uint16_t col = 0; col < schema->column_count && i + 1 < argc; col++) {
                i++;
                char *value = argv[i];

                // Handle NULL values
                if (!strcmp(value, "NULL")) {
                    record->fields[col].is_null = 1;
                    continue;
                }

                record->fields[col].is_null = 0;
                record->fields[col].type = schema->columns[col].type;

                switch (schema->columns[col].type) {
                    case COL_INT:
                        record->fields[col].value.int_val = atoi(value);
                        break;
                    case COL_BOOL:
                        record->fields[col].value.bool_val = (!strcmp(value, "true") || !strcmp(value, "1")) ? 1 : 0;
                        break;
                    case COL_TEXT:
                        strncpy(record->fields[col].value.text_val, value, MAX_RECORD_VALUE_SIZE - 1);
                        break;
                }
            }

            uint64_t record_id = insertRecord(db, record);
            if (record_id > 0) {
                flushAllDirtyPages(db->buffer_pool, db);
                writeHeader(db);
                printf("Record inserted with ID %lu\n", record_id);
            } else {
                fprintf(stderr, "Failed to insert record\n");
            }

            freeRecord(record);
            free(schema);
            freeDatabase(db);
            exit(0);

        } else if (!strcmp(argv[i], "-read-record")) {
            // Read a specific record
            // Usage: -read-record <db_path> <table_id> <record_id>
            if (i + 3 >= argc) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: database path required\n");
                exit(1);
            }
                fprintf(stderr, "Usage: -read-record <db_path> <table_id> <record_id>\n");
                exit(1);
            }

            char *path = appendFileExt(argv[++i]);
            uint16_t table_id = (uint16_t)atoi(argv[++i]);
            uint64_t record_id = (uint64_t)atoll(argv[++i]);

            FILE *dbFile = fopen(path, "r+b");
            if (!dbFile) {
                fprintf(stderr, "Failed to open database file\n");
                exit(1);
            }

            Header *header = malloc(sizeof(Header));
            if (fread(header, sizeof(Header), 1, dbFile) != 1) {
                fprintf(stderr, "Failed to read database header\n");
                fclose(dbFile);
                exit(1);
            }

            MagBase *db = createMagBase(header, path, false);
            TableSchemaRecord *schema = readTableSchema(db, table_id);
            if (!schema) {
                fprintf(stderr, "Table not found\n");
                freeDatabase(db);
                exit(1);
            }

            Record *record = readRecord(db, table_id, record_id);
            if (record) {
                printf("Record ID %lu:\n", record->record_id);
                for (uint16_t col = 0; col < record->field_count; col++) {
                    printf("  %s: ", schema->columns[col].name);
                    if (record->fields[col].is_null) {
                        printf("NULL\n");
                    } else {
                        switch (record->fields[col].type) {
                            case COL_INT:
                                printf("%d\n", record->fields[col].value.int_val);
                                break;
                            case COL_BOOL:
                                printf("%s\n", record->fields[col].value.bool_val ? "true" : "false");
                                break;
                            case COL_TEXT:
                                printf("%s\n", record->fields[col].value.text_val);
                                break;
                        }
                    }
                }
                freeRecord(record);
            } else {
                fprintf(stderr, "Record not found\n");
            }

            free(schema);
            freeDatabase(db);
            exit(0);

        } else if (!strcmp(argv[i], "-list-records")) {
            // List all records in a table
            // Usage: -list-records <db_path> <table_id>
            if (i + 2 >= argc) {
                fprintf(stderr, "Usage: -list-records <db_path> <table_id>\n");
                exit(1);
            }

            if (i + 1 >= argc) {
                fprintf(stderr, "Error: database path required\n");
                exit(1);
            }
            
            // Create path buffer on stack to avoid corrupting argv
            char path_buffer[256];
            strncpy(path_buffer, argv[++i], sizeof(path_buffer) - 5);
            path_buffer[sizeof(path_buffer) - 5] = '\0';
            if (strlen(path_buffer) < 4 || strcmp(&path_buffer[strlen(path_buffer) - 4], ".mab") != 0) {
                strcat(path_buffer, ".mab");
            }
            
            uint16_t table_id = (uint16_t)atoi(argv[++i]);

            FILE *dbFile = fopen(path_buffer, "r+b");
            if (!dbFile) {
                fprintf(stderr, "Failed to open database file\n");
                exit(1);
            }

            Header *header = malloc(sizeof(Header));
            if (fread(header, sizeof(Header), 1, dbFile) != 1) {
                fprintf(stderr, "Failed to read database header\n");
                fclose(dbFile);
                exit(1);
            }

            MagBase *db = createMagBase(header, path_buffer, false);
            TableSchemaRecord *schema = readTableSchema(db, table_id);
            if (!schema) {
                fprintf(stderr, "Table not found\n");
                freeDatabase(db);
                exit(1);
            }

            uint64_t num_records = 0;
            Record **records = readAllRecords(db, table_id, &num_records);

            if (num_records == 0) {
                printf("No records found\n");
            } else {
                printf("Records in table %d:\n", table_id);
                for (uint64_t r = 0; r < num_records; r++) {
                    printf("  [ID %lu] ", records[r]->record_id);
                    for (uint16_t col = 0; col < records[r]->field_count; col++) {
                        if (records[r]->fields[col].is_null) {
                            printf("NULL");
                        } else {
                            switch (records[r]->fields[col].type) {
                                case COL_INT:
                                    printf("%d", records[r]->fields[col].value.int_val);
                                    break;
                                case COL_BOOL:
                                    printf("%s", records[r]->fields[col].value.bool_val ? "true" : "false");
                                    break;
                                case COL_TEXT:
                                    printf("%s", records[r]->fields[col].value.text_val);
                                    break;
                            }
                        }
                        if (col < records[r]->field_count - 1) printf(" | ");
                    }
                    printf("\n");
                    freeRecord(records[r]);
                }
                free(records);
            }

            free(schema);
            freeDatabase(db);
            exit(0);

        } else if (!strcmp(argv[i], "-update-record")) {
            // Update an existing record
            // Usage: -update-record <db_path> <table_id> <record_id> [field_value ...]
            if (i + 3 >= argc) {
                fprintf(stderr, "Usage: -update-record <db_path> <table_id> <record_id> [field_value ...]\n");
                exit(1);
            }

            if (i + 1 >= argc) {
                fprintf(stderr, "Error: database path required\n");
                exit(1);
            }
            char *path = appendFileExt(argv[++i]);
            uint16_t table_id = (uint16_t)atoi(argv[++i]);
            uint64_t record_id = (uint64_t)atoll(argv[++i]);

            FILE *dbFile = fopen(path, "r+b");
            if (!dbFile) {
                fprintf(stderr, "Failed to open database file\n");
                exit(1);
            }

            Header *header = malloc(sizeof(Header));
            if (fread(header, sizeof(Header), 1, dbFile) != 1) {
                fprintf(stderr, "Failed to read database header\n");
                fclose(dbFile);
                exit(1);
            }

            MagBase *db = createMagBase(header, path, false);
            TableSchemaRecord *schema = readTableSchema(db, table_id);
            if (!schema) {
                fprintf(stderr, "Table not found\n");
                freeDatabase(db);
                exit(1);
            }

            Record *record = readRecord(db, table_id, record_id);
            if (!record) {
                fprintf(stderr, "Record not found\n");
                free(schema);
                freeDatabase(db);
                exit(1);
            }

            // Update fields
            for (uint16_t col = 0; col < schema->column_count && i + 1 < argc; col++) {
                i++;
                char *value = argv[i];

                if (!strcmp(value, "NULL")) {
                    record->fields[col].is_null = 1;
                    continue;
                }

                record->fields[col].is_null = 0;
                record->fields[col].type = schema->columns[col].type;

                switch (schema->columns[col].type) {
                    case COL_INT:
                        record->fields[col].value.int_val = atoi(value);
                        break;
                    case COL_BOOL:
                        record->fields[col].value.bool_val = (!strcmp(value, "true") || !strcmp(value, "1")) ? 1 : 0;
                        break;
                    case COL_TEXT:
                        strncpy(record->fields[col].value.text_val, value, MAX_RECORD_VALUE_SIZE - 1);
                        break;
                }
            }

            int result = updateRecord(db, record);
            if (result == 0) {
                flushAllDirtyPages(db->buffer_pool, db);
                writeHeader(db);
                printf("Record updated\n");
            } else {
                fprintf(stderr, "Failed to update record\n");
            }

            freeRecord(record);
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: database path required\n");
                exit(1);
            }
            free(schema);
            freeDatabase(db);
            exit(0);

        } else if (!strcmp(argv[i], "-delete-record")) {
            // Delete a record
            // Usage: -delete-record <db_path> <table_id> <record_id>
            if (i + 3 >= argc) {
                fprintf(stderr, "Usage: -delete-record <db_path> <table_id> <record_id>\n");
                exit(1);
            }

            char *path = appendFileExt(argv[++i]);
            uint16_t table_id = (uint16_t)atoi(argv[++i]);
            uint64_t record_id = (uint64_t)atoll(argv[++i]);

            FILE *dbFile = fopen(path, "r+b");
            if (!dbFile) {
                fprintf(stderr, "Failed to open database file\n");
                exit(1);
            }

            Header *header = malloc(sizeof(Header));
            if (fread(header, sizeof(Header), 1, dbFile) != 1) {
                fprintf(stderr, "Failed to read database header\n");
                fclose(dbFile);
                exit(1);
            }

            MagBase *db = createMagBase(header, path, false);

            int result = deleteRecord(db, table_id, record_id);
            if (result == 0) {
                flushAllDirtyPages(db->buffer_pool, db);
                writeHeader(db);
                printf("Record deleted\n");
            } else {
                fprintf(stderr, "Record not found\n");
            }

            freeDatabase(db);
            exit(0);
        }
    }
}
