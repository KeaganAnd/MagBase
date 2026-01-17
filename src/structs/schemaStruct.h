#include <stdint.h>
#define MAX_COLUMN_NAME 32
#define MAX_TABLE_NAME 32
#define MAX_COLUMNS 16

typedef enum { COL_INT, COL_TEXT, COL_BOOL } ColumnType;

typedef struct {
    uint8_t type;               // ColumnType
    uint8_t nullable;           // 0 or 1
    uint16_t name_len;          // actual length
    char name[MAX_COLUMN_NAME]; // fixed storage
} SchemaColumn;

typedef struct {
    uint16_t table_id;
    uint16_t column_count;
    uint32_t root_page;
    uint16_t name_len;
    char table_name[MAX_TABLE_NAME];
    SchemaColumn columns[MAX_COLUMNS];
} TableSchemaRecord;

typedef struct {
    uint16_t table_count;
    uint16_t free_space_offset;
    uint32_t next_schema_page;
} SchemaPageHeader;
