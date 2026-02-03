//     Keagan Anderson
//        MagBase
//       02/03/2026
//
//     Record management functions for reading/writing/modifying table records

#pragma once

#include "db-init.h"
#include "structs/schemaStruct.h"
#include <stdint.h>

// Maximum size for a record value (for text fields)
#define MAX_RECORD_VALUE_SIZE 256

// A record field containing a value
typedef struct {
    uint8_t type;                       // ColumnType (COL_INT, COL_TEXT, COL_BOOL)
    uint8_t is_null;                    // 1 if NULL, 0 if has value
    union {
        int32_t int_val;
        char text_val[MAX_RECORD_VALUE_SIZE];
        uint8_t bool_val;
    } value;
} RecordField;

// A complete record with all field values
typedef struct {
    uint64_t record_id;                 // Unique record identifier
    uint16_t table_id;                  // Which table this record belongs to
    RecordField *fields;                // Array of fields matching table schema
    uint16_t field_count;               // Number of fields
} Record;

// Create a new empty record for a table
// Returns a pointer to the record, caller must free it
Record *createRecord(uint16_t table_id, uint16_t field_count);

// Free a record from memory
void freeRecord(Record *record);

// Insert a record into a table
// Returns the record_id of the inserted record, or 0 on error
uint64_t insertRecord(MagBase *db, Record *record);

// Read a record by record_id and table_id
// Returns a pointer to the record (allocated), or NULL if not found
// Caller must free the returned record
Record *readRecord(MagBase *db, uint16_t table_id, uint64_t record_id);

// Update an existing record
// Returns 0 on success, -1 on error
int updateRecord(MagBase *db, Record *record);

// Delete a record by record_id and table_id
// Returns 0 on success, -1 on error
int deleteRecord(MagBase *db, uint16_t table_id, uint64_t record_id);

// Read all records from a table
// Returns an array of Record pointers
// num_records is set to the count of records found
// Caller must free each record and the array itself
Record **readAllRecords(MagBase *db, uint16_t table_id, uint64_t *num_records);
