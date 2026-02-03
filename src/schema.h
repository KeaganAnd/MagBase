//     Keagan Anderson
//        MagBase
//       02/03/2026
//
//     Schema management functions for reading/writing table schemas

#pragma once

#include "db-init.h"
#include "structs/schemaStruct.h"
#include <stdint.h>

// Write a table schema to the schema pages
// Returns the table_id of the written schema, or -1 on error
int writeTableSchema(MagBase *db, TableSchemaRecord *schema);

// Read a table schema by table_id
// Returns a pointer to the schema (allocated), or NULL if not found
// Caller must free the returned pointer
TableSchemaRecord *readTableSchema(MagBase *db, uint16_t table_id);

// Read all table schemas from the schema pages
// Returns an array of TableSchemaRecord pointers
// num_tables is set to the count of tables found
// Caller must free each pointer in the array and the array itself
TableSchemaRecord **readAllTableSchemas(MagBase *db, uint16_t *num_tables);

// Delete a table schema by table_id
// Returns 0 on success, -1 on error
int deleteTableSchema(MagBase *db, uint16_t table_id);

// Add a column to an existing table schema
// Returns 0 on success, -1 on error
int addColumnToTable(MagBase *db, uint16_t table_id, SchemaColumn *column);

// Remove a column from a table schema
// Returns 0 on success, -1 on error
int removeColumnFromTable(MagBase *db, uint16_t table_id, uint16_t column_index);

// Update a column in a table schema
// Returns 0 on success, -1 on error
int updateTableColumn(MagBase *db, uint16_t table_id, uint16_t column_index, SchemaColumn *new_column);
