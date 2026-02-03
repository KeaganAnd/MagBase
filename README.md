# MagBase Command Reference

MagBase is a toy database system. This document provides comprehensive documentation for all available command-line flags and operations.

## Table of Contents
1. [Basic Commands](#basic-commands)
2. [Database Operations](#database-operations)
3. [Table Operations](#table-operations)
4. [Record Operations](#record-operations)
5. [Data Types](#data-types)
6. [Examples](#examples)

---

## Basic Commands

### `-v` / `-version`
Display the current version of MagBase.

**Syntax:**
```bash
magbase -v
magbase -version
```

**Output:**
```
Version 1.0.0
```

**Description:**
Shows the semantic version of the MagBase application. Useful for verifying which version is installed.

---

### `-h` / `-help`
Display help information about MagBase.

**Syntax:**
```bash
magbase -h
magbase -help
```

**Output:**
Displays the contents of the help file with general information about MagBase.

**Description:**
Shows usage instructions and general help information. The help content is read from `docs/help-page.txt`.

---

## Database Operations

### `-p` (Open/Create Database)
Open an existing database or create a new one if it doesn't exist.

**Syntax:**
```bash
magbase -p <database_path>
```

**Parameters:**
- `<database_path>`: Path to the database file. The `.mab` extension is automatically added if not present.

**Examples:**
```bash
# Create or open a database named 'mydb'
magbase -p mydb

# Open a database with full path
magbase -p /home/user/databases/mydb

# Explicitly use .mab extension
magbase -p mydb.mab
```

**Description:**
- If the database file doesn't exist, MagBase initializes a new database with proper headers and structure
- If the file exists, MagBase verifies it's a valid MagBase database and displays version information
- Creates initial schema and free list pages automatically
- The operation verifies file integrity by checking the magic bytes (`MAGDB.\0\0`)

**Notes:**
- A new database starts with 2 pages (header page + schema root page)
- The schema root page is reserved for storing table definitions
- Free list tracking begins at page 2

---

## Table Operations

### `-create-table` (Create a New Table)
Define and create a new table schema in the database.

**Syntax:**
```bash
magbase -create-table <db_path> <table_name> <num_columns> [col_name:type:nullable ...]
```

**Parameters:**
- `<db_path>`: Path to the database file (`.mab` extension added automatically)
- `<table_name>`: Name of the table (max 32 characters)
- `<num_columns>`: Number of columns to create (1-16)
- `[col_name:type:nullable ...]`: Column definitions in format `name:type:nullable`
  - `name`: Column name (max 32 characters)
  - `type`: Data type - `int`, `text`, or `bool`
  - `nullable`: `0` for NOT NULL, `1` for nullable

**Examples:**
```bash
# Create a users table
magbase -create-table mydb users 3 id:int:0 name:text:0 active:bool:1

# Create a products table
magbase -create-table mydb products 4 product_id:int:0 title:text:0 price:int:0 in_stock:bool:0

# Create a minimal table
magbase -create-table mydb simple 1 value:text:0
```

**Output:**
```
Table 'users' created with ID 1
```

**Description:**
- Automatically assigns a unique table ID (sequential starting from 1)
- Stores the complete schema definition including column types and nullability
- Schema is stored in special schema pages with automatic page allocation
- Each table gets a separate data page chain starting from the allocated root page

**Constraints:**
- Maximum 16 columns per table
- Table names limited to 32 characters
- Column names limited to 32 characters
- Text values limited to 256 bytes per field

---

### `-list-tables` (View All Tables)
Display all tables in a database with their columns.

**Syntax:**
```bash
magbase -list-tables <db_path>
```

**Parameters:**
- `<db_path>`: Path to the database file (`.mab` extension added automatically)

**Examples:**
```bash
magbase -list-tables mydb
```

**Output:**
```
Tables in database:
  [ID 1] users (3 columns)
    - id (int)
    - name (text)
    - active (bool) [nullable]
  [ID 2] products (4 columns)
    - product_id (int)
    - title (text)
    - price (int)
    - in_stock (bool)
```

**Description:**
- Lists all table schemas currently defined in the database
- Shows table ID, name, and column count
- Displays each column with its type and nullability constraints
- Useful for reviewing database structure

---

### `-delete-table` (Remove a Table)
Delete a table schema from the database.

**Syntax:**
```bash
magbase -delete-table <db_path> <table_id>
```

**Parameters:**
- `<db_path>`: Path to the database file (`.mab` extension added automatically)
- `<table_id>`: The ID of the table to delete (shown in `-list-tables`)

**Examples:**
```bash
# Delete table with ID 2
magbase -delete-table mydb 2
```

**Output:**
```
Table with ID 2 deleted
```

**Description:**
- Removes the table schema definition from the schema pages
- Automatically compacts schema storage by shifting remaining records
- Does NOT automatically delete table data records (data cleanup would need to be implemented separately)
- Table IDs are not reused

**Warning:**
- Ensure no dependent queries or code reference the deleted table before deletion

---

## Record Operations

### `-insert-record` (Add a New Record)
Insert a new record into a table.

**Syntax:**
```bash
magbase -insert-record <db_path> <table_id> [field_value ...]
```

**Parameters:**
- `<db_path>`: Path to the database file (`.mab` extension added automatically)
- `<table_id>`: The ID of the target table
- `[field_value ...]`: Values for each column in table definition order
  - Use `NULL` for NULL values in nullable columns
  - For bool fields: use `true`/`false` or `1`/`0`

**Examples:**
```bash
# Insert a user record
magbase -insert-record mydb 1 123 "John Doe" true

# Insert with NULL value
magbase -insert-record mydb 1 456 "Jane Smith" NULL

# Insert a product record
magbase -insert-record mydb 2 1001 "Laptop" 999 true
```

**Output:**
```
Record inserted with ID 1
```

**Description:**
- Creates a new record with auto-assigned record ID
- Records are stored in data pages linked to the table's root page
- Automatically allocates new data pages when current page is full
- Values are validated against their column types

**Notes:**
- Record IDs start from 1 and increment sequentially
- Text values are limited to 256 bytes
- Integer values are stored as 32-bit signed integers
- Boolean values stored as 0 or 1

---

### `-read-record` (Retrieve a Specific Record)
Fetch and display a single record by its ID.

**Syntax:**
```bash
magbase -read-record <db_path> <table_id> <record_id>
```

**Parameters:**
- `<db_path>`: Path to the database file (`.mab` extension added automatically)
- `<table_id>`: The ID of the table
- `<record_id>`: The ID of the record to retrieve

**Examples:**
```bash
# Read user record ID 1
magbase -read-record mydb 1 1

# Read product record ID 3
magbase -read-record mydb 2 3
```

**Output:**
```
Record ID 1:
  id: 123
  name: John Doe
  active: true
```

**Description:**
- Searches through all data pages of the table for the matching record
- Displays field names alongside values for clarity
- Shows NULL values clearly marked as `NULL`
- Boolean values displayed as `true` or `false`

---

### `-list-records` (View All Records in a Table)
Display all records from a specific table.

**Syntax:**
```bash
magbase -list-records <db_path> <table_id>
```

**Parameters:**
- `<db_path>`: Path to the database file (`.mab` extension added automatically)
- `<table_id>`: The ID of the table to read

**Examples:**
```bash
# List all user records
magbase -list-records mydb 1

# List all products
magbase -list-records mydb 2
```

**Output:**
```
Records in table 1:
  [ID 1] 123 | John Doe | true
  [ID 2] 456 | Jane Smith | NULL
  [ID 3] 789 | Bob Johnson | true
```

**Description:**
- Retrieves all records stored in the table
- Displays records in a compact pipe-delimited format
- Shows record ID in brackets for reference
- Useful for quick data inspection and validation

**Notes:**
- NULL values displayed as `NULL`
- Boolean values shown as `true` or `false`
- Performance depends on number of records and pages

---

### `-update-record` (Modify an Existing Record)
Change field values in an existing record.

**Syntax:**
```bash
magbase -update-record <db_path> <table_id> <record_id> [field_value ...]
```

**Parameters:**
- `<db_path>`: Path to the database file (`.mab` extension added automatically)
- `<table_id>`: The ID of the table
- `<record_id>`: The ID of the record to update
- `[field_value ...]`: New values for each column in table definition order
  - Use `NULL` for NULL values in nullable columns
  - For bool fields: use `true`/`false` or `1`/`0`

**Examples:**
```bash
# Update user record 1
magbase -update-record mydb 1 1 123 "John Updated" false

# Update product inventory
magbase -update-record mydb 2 1 1001 "Laptop" 899 false
```

**Output:**
```
Record updated
```

**Description:**
- Locates the record by ID and table ID
- Replaces all field values with provided values
- Current implementation deletes old record and inserts updated one
- Maintains data integrity by validating against schema

**Notes:**
- All fields must be provided (partial updates not supported)
- Record ID remains the same after update
- Type validation enforced on all field values

---

### `-delete-record` (Remove a Record)
Delete a record from a table.

**Syntax:**
```bash
magbase -delete-record <db_path> <table_id> <record_id>
```

**Parameters:**
- `<db_path>`: Path to the database file (`.mab` extension added automatically)
- `<table_id>`: The ID of the table
- `<record_id>`: The ID of the record to delete

**Examples:**
```bash
# Delete user record 2
magbase -delete-record mydb 1 2

# Delete product record 5
magbase -delete-record mydb 2 5
```

**Output:**
```
Record deleted
```

**Description:**
- Finds and removes the record from the table
- Automatically compacts the data page by shifting remaining records
- Frees the space used by the deleted record for future use
- Record ID is not reused

**Notes:**
- Deletion is permanent once confirmed
- Freed space is immediately available for new records
- If record is not found, returns "Record not found" error

---

## Data Types

MagBase supports three fundamental data types:

### `int`
- **Description**: 32-bit signed integer
- **Range**: -2,147,483,648 to 2,147,483,647
- **Storage**: 4 bytes
- **Example**: `123`, `-456`, `0`

### `text`
- **Description**: Variable-length text string
- **Maximum Length**: 256 bytes per field
- **Storage**: 2 bytes (length) + string bytes
- **Example**: `"Hello World"`, `"Jane Doe"`, `"Product Name"`

### `bool`
- **Description**: Boolean true/false value
- **Values**: `true`/`false` or `1`/`0`
- **Storage**: 1 byte
- **Example**: `true`, `false`, `1`, `0`

### NULL Values
- Available on columns marked as nullable (`:1` in column definition)
- Specified using the keyword `NULL`
- Not applicable to NOT NULL columns (`:0` in column definition)
- Example: `magbase -insert-record mydb 1 123 "John" NULL`

---

## Examples

### Complete Workflow Example

```bash
# 1. Create a database
magbase -p myshop

# 2. Create a customers table
magbase -create-table myshop customers 3 \
  customer_id:int:0 name:text:0 premium:bool:0

# 3. Create a products table
magbase -create-table myshop products 4 \
  product_id:int:0 name:text:0 price:int:0 available:bool:0

# 4. View all tables
magbase -list-tables myshop

# 5. Insert customer records
magbase -insert-record myshop 1 1001 "Alice Johnson" true
magbase -insert-record myshop 1 1002 "Bob Smith" false
magbase -insert-record myshop 1 1003 "Charlie Brown" true

# 6. Insert product records
magbase -insert-record myshop 2 5001 "Laptop" 999 true
magbase -insert-record myshop 2 5002 "Mouse" 25 true
magbase -insert-record myshop 2 5003 "Keyboard" 79 false

# 7. View all customer records
magbase -list-records myshop 1

# 8. View specific customer
magbase -read-record myshop 1 1

# 9. Update a product price
magbase -update-record myshop 2 2 5002 "Mouse" 29 true

# 10. Delete a customer
magbase -delete-record myshop 1 2

# 11. List remaining customers
magbase -list-records myshop 1
```

### Schema Modification Example

```bash
# Create a table
magbase -create-table mydb users 2 id:int:0 email:text:0

# View the table
magbase -list-tables mydb

# Insert some records
magbase -insert-record mydb 1 1 "user1@example.com"
magbase -insert-record mydb 1 2 "user2@example.com"

# Delete the table if needed
magbase -delete-table mydb 1

# Create a new improved schema
magbase -create-table mydb users 4 \
  id:int:0 email:text:0 name:text:0 verified:bool:1

# Insert new records with updated schema
magbase -insert-record mydb 2 1 "user1@example.com" "User One" true
magbase -insert-record mydb 2 2 "user2@example.com" "User Two" NULL
```

---