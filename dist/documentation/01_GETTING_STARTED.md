# Getting Started with LyraDB

## Introduction

LyraDB is a lightweight, in-memory columnar database engine written in C++. It provides a SQL interface for creating tables, inserting data, querying, updating, and deleting records. The database supports both C and C++ APIs for easy integration into your applications.

## Version

LyraDB v1.2.0

## System Requirements

- Operating System: Windows, Linux, macOS
- C++ Compiler: C++11 or later (GCC 4.8+, Clang 3.3+, MSVC 2015+)
- Build System: CMake 3.10 or later
- Memory: Minimum 512MB RAM
- Disk Space: 50MB for source code and build artifacts

## Installation

### Build from Source

1. Clone or extract the LyraDB source code:

```bash
cd LyraDB
```

2. Create a build directory:

```bash
mkdir build
cd build
```

3. Configure the build with CMake:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

4. Build the project:

On Windows:
```batch
msbuild lyradb.sln /p:Configuration=Release /p:Platform=x64
```

On Linux/macOS:
```bash
make
```

5. Verify the installation by running tests:

On Windows:
```batch
.\Release\comprehensive_test.exe
.\Release\music_test.exe
```

On Linux/macOS:
```bash
./Release/comprehensive_test
./Release/music_test
```

## Quick Start

### Using the C API

```c
#include <stdio.h>
#include "lyradb_c.h"

int main() {
    // Create database
    lyradb_handle_t db = lyradb_create_database();
    
    // Create table
    const char* sql = "CREATE TABLE users (id INT32, name STRING, age INT32)";
    lyradb_result_t result = lyradb_execute(db, sql);
    
    // Insert data
    sql = "INSERT INTO users VALUES (1, 'John', 30)";
    result = lyradb_execute(db, sql);
    
    // Query data
    sql = "SELECT * FROM users";
    result = lyradb_execute(db, sql);
    
    // Clean up
    lyradb_destroy_database(db);
    
    return 0;
}
```

### Using the C++ API

```cpp
#include "lyradb/database.h"
#include "lyradb/sql_parser.h"

int main() {
    // Create database
    lyradb::Database db;
    
    // Create parser
    auto parser = lyradb::SQLParser::create();
    
    // Parse and execute CREATE TABLE
    auto stmt = parser->parse("CREATE TABLE users (id INT32, name STRING, age INT32)");
    
    // Parse and execute INSERT
    stmt = parser->parse("INSERT INTO users VALUES (1, 'John', 30)");
    
    // Parse and execute SELECT
    stmt = parser->parse("SELECT * FROM users");
    
    return 0;
}
```

## Basic Concepts

### Tables

A table is a collection of columns and rows. Each column has a name and a data type. Tables are created using the CREATE TABLE statement.

### Data Types

LyraDB supports the following data types:

- INT32: 32-bit signed integer (-2,147,483,648 to 2,147,483,647)
- INT64: 64-bit signed integer
- FLOAT32: 32-bit floating-point number
- FLOAT64: 64-bit floating-point number
- STRING: Text data of variable length
- BOOL: Boolean value (true/false)

### SQL Statements

LyraDB supports the following SQL statements:

- CREATE TABLE: Define a new table
- INSERT: Add rows to a table
- SELECT: Query data from a table
- UPDATE: Modify existing rows
- DELETE: Remove rows from a table
- CREATE INDEX: Create an index for optimization
- DROP TABLE: Remove a table from the database
- DROP INDEX: Remove an index

## Common Tasks

### Creating a Table

```sql
CREATE TABLE products (
    id INT32,
    name STRING,
    price FLOAT64,
    stock INT32
)
```

### Inserting Data

```sql
INSERT INTO products VALUES (1, 'Laptop', 999.99, 10)
INSERT INTO products (id, name, price) VALUES (2, 'Mouse', 29.99)
```

### Querying Data

```sql
SELECT * FROM products
SELECT name, price FROM products WHERE price > 100
```

### Updating Data

```sql
UPDATE products SET stock = 5 WHERE id = 1
```

### Deleting Data

```sql
DELETE FROM products WHERE id = 1
```

### Creating an Index

```sql
CREATE INDEX idx_names ON products (name)
```

### Dropping a Table

```sql
DROP TABLE products
```

## Error Handling

LyraDB provides detailed error messages including:

- Error message describing what went wrong
- Location (line number and column number)
- The problematic token
- Helpful hints for correction

Example error message:

```
SQL Syntax Error:
  Message: Unexpected token
  Location: Line 1, Column 15
  Token: '('
  Hint: Expected SELECT, INSERT, UPDATE, DELETE, CREATE, or DROP keyword
```

## Performance Considerations

1. Use indexes on columns frequently used in WHERE clauses
2. Avoid selecting unnecessary columns
3. Use WHERE clauses to filter data early
4. Keep tables reasonably sized for optimal performance

## Troubleshooting

### Build Errors

If you encounter build errors:

1. Ensure CMake version is 3.10 or later
2. Check that a compatible C++ compiler is installed
3. Verify all dependencies are satisfied
4. Clean the build directory and rebuild: `cmake --build . --target clean`

### Runtime Errors

If you encounter runtime errors:

1. Check that SQL syntax is correct
2. Verify that table and column names exist
3. Ensure data types match column definitions
4. Check error messages for specific details

### Connection Issues

If the database fails to open:

1. Verify sufficient disk space
2. Check file permissions
3. Ensure the database path is valid

## Next Steps

- Read the SQL Reference guide for detailed syntax information
- Review the API Reference for programming examples
- Check the Examples directory for sample applications
- Consult Troubleshooting guide for common issues

## Support

For issues or questions:

1. Check the ISSUES_FIXED.md file for known issues
2. Review example code in the examples directory
3. Examine test files for usage patterns
