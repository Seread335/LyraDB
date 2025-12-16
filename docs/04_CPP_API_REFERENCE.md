# C++ API Reference

## Overview

The C++ API provides an object-oriented interface to LyraDB. Key classes include Database, SQLParser, and QueryResult.

## Core Classes

### Database Class

Main database engine class for executing SQL operations.

Header:
```cpp
#include "lyradb/database.h"
```

Namespace:
```cpp
namespace lyradb {
    class Database { ... };
}
```

Methods:

#### Constructor

```cpp
Database();
```

Creates a new database instance. The database starts empty and is ready for table creation.

Example:
```cpp
lyradb::Database db;
```

#### Destructor

```cpp
~Database();
```

Automatically closes the database and releases resources.

#### execute

```cpp
std::unique_ptr<QueryResult> execute(const std::shared_ptr<Statement>& stmt);
```

Parameters:

- stmt: Parsed SQL statement from SQLParser

Return Value:

- std::unique_ptr<QueryResult>: Result object or nullptr on error

Description:

Executes a parsed SQL statement. Returns a QueryResult object containing the results, or nullptr if an error occurred.

Example:
```cpp
auto result = db.execute(parsed_stmt);
if (result) {
    int row_count = result->row_count();
}
```

---

### SQLParser Class

Parser for converting SQL strings to executable statements.

Header:
```cpp
#include "lyradb/sql_parser.h"
```

Namespace:
```cpp
namespace lyradb {
    class SQLParser { ... };
}
```

Static Methods:

#### create

```cpp
static std::shared_ptr<SQLParser> create();
```

Return Value:

- std::shared_ptr<SQLParser>: Shared pointer to new parser instance

Description:

Factory method that creates a new SQLParser instance.

Example:
```cpp
auto parser = lyradb::SQLParser::create();
```

Instance Methods:

#### parse

```cpp
std::shared_ptr<Statement> parse(const std::string& sql);
```

Parameters:

- sql: SQL statement string to parse

Return Value:

- std::shared_ptr<Statement>: Parsed statement object, or nullptr if parse error

Description:

Parses a SQL string into a Statement object. If parsing fails, returns nullptr. Call error() to get error details.

Example:
```cpp
auto stmt = parser->parse("CREATE TABLE users (id INT32, name STRING)");
if (!stmt) {
    std::cerr << "Parse error: " << parser->error() << std::endl;
    return;
}
```

#### error

```cpp
std::string error() const;
```

Return Value:

- std::string: Error message from last parse attempt

Description:

Returns detailed error information including location and hints if the last parse operation failed.

Example:
```cpp
if (!stmt) {
    std::cout << parser->error() << std::endl;
}
```

#### detailed_error

```cpp
std::string detailed_error() const;
```

Return Value:

- std::string: Detailed error message with context

Description:

Returns comprehensive error information including line number, column, token, and helpful hints.

Example:
```cpp
std::cout << parser->detailed_error() << std::endl;
```

---

### QueryResult Class

Base class for query results. Use EngineQueryResult for actual results.

Header:
```cpp
#include "lyradb/query_result.h"
```

Namespace:
```cpp
namespace lyradb {
    class QueryResult { ... };
    class EngineQueryResult : public QueryResult { ... };
}
```

Methods:

#### row_count

```cpp
virtual int row_count() const = 0;
```

Return Value:

- int: Number of rows in the result

#### column_count

```cpp
virtual int column_count() const = 0;
```

Return Value:

- int: Number of columns in the result

#### column_names

```cpp
virtual std::vector<std::string> column_names() const = 0;
```

Return Value:

- std::vector<std::string>: Vector of column names

#### get_value

```cpp
virtual std::string get_value(int row, int col) const = 0;
```

Parameters:

- row: Row index (0-based)
- col: Column index (0-based)

Return Value:

- std::string: Value as string

#### Type-Safe Getters

```cpp
int get_int(int row, int col) const;
double get_double(int row, int col) const;
std::string get_string(int row, int col) const;
bool get_bool(int row, int col) const;
```

Description:

Retrieve values with automatic type conversion. Returns 0/0.0/""/false if conversion fails.

Example:
```cpp
int id = result->get_int(0, 0);
std::string name = result->get_string(0, 1);
double salary = result->get_double(0, 2);
```

---

### EngineQueryResult Class

Concrete implementation of QueryResult for in-memory result sets.

Constructors:

```cpp
EngineQueryResult(const std::vector<std::string>& column_names);

EngineQueryResult(
    const std::vector<std::vector<std::string>>& rows,
    const std::vector<std::string>& column_names);
```

Methods:

#### add_row

```cpp
void add_row(const std::vector<std::string>& row);
```

Parameters:

- row: Vector of string values to add as a row

#### get_rows

```cpp
std::vector<std::vector<std::string>> get_rows() const;
```

Return Value:

- std::vector<std::vector<std::string>>: All rows in the result

Example:
```cpp
auto result = std::make_unique<EngineQueryResult>(column_names);
result->add_row({"1", "Alice", "75000"});
result->add_row({"2", "Bob", "85000"});

for (int i = 0; i < result->row_count(); ++i) {
    std::string name = result->get_string(i, 1);
    double salary = result->get_double(i, 2);
}
```

---

## Statement Classes

### CreateTableStatement

Represents a CREATE TABLE statement.

Data Members:

- table_name: std::string - Name of the table
- columns: std::vector<Column> - Column definitions

---

### InsertStatement

Represents an INSERT statement.

Data Members:

- table_name: std::string - Target table name
- values: std::vector<Expression> - Values to insert

---

### SelectStatement

Represents a SELECT statement.

Data Members:

- table_name: std::string - Source table name
- columns: std::vector<std::string> - Selected columns
- where_expr: Expression* - Optional WHERE condition

---

### UpdateStatement

Represents an UPDATE statement.

Data Members:

- table_name: std::string - Target table name
- assignments: std::map<std::string, Expression> - Column assignments
- where_expr: Expression* - Optional WHERE condition

---

### DeleteStatement

Represents a DELETE statement.

Data Members:

- table_name: std::string - Target table name
- where_expr: Expression* - Optional WHERE condition

---

### CreateIndexStatement

Represents a CREATE INDEX statement.

Data Members:

- index_name: std::string - Name of the index
- table_name: std::string - Target table name
- columns: std::vector<std::string> - Columns to index

---

### DropStatement

Represents a DROP TABLE or DROP INDEX statement.

Data Members:

- statement_type: StatementType - DROP_TABLE or DROP_INDEX
- object_name: std::string - Name of object to drop
- if_exists: bool - Whether IF EXISTS was specified

---

## Usage Examples

### Example 1: Create and Populate a Table

```cpp
#include <iostream>
#include "lyradb/database.h"
#include "lyradb/sql_parser.h"

int main() {
    lyradb::Database db;
    auto parser = lyradb::SQLParser::create();
    
    // Create table
    auto stmt = parser->parse(
        "CREATE TABLE users (id INT32, name STRING, age INT32)");
    auto result = db.execute(stmt);
    
    // Insert data
    stmt = parser->parse(
        "INSERT INTO users VALUES (1, 'Alice', 30)");
    result = db.execute(stmt);
    
    stmt = parser->parse(
        "INSERT INTO users VALUES (2, 'Bob', 25)");
    result = db.execute(stmt);
    
    return 0;
}
```

### Example 2: Query and Display Results

```cpp
#include <iostream>
#include "lyradb/database.h"
#include "lyradb/sql_parser.h"

int main() {
    lyradb::Database db;
    auto parser = lyradb::SQLParser::create();
    
    // Create and populate table
    auto stmt = parser->parse(
        "CREATE TABLE products (id INT32, name STRING, price FLOAT64)");
    db.execute(stmt);
    
    stmt = parser->parse(
        "INSERT INTO products VALUES (1, 'Laptop', 999.99)");
    db.execute(stmt);
    
    stmt = parser->parse(
        "INSERT INTO products VALUES (2, 'Mouse', 29.99)");
    db.execute(stmt);
    
    // Query data
    stmt = parser->parse(
        "SELECT id, name, price FROM products WHERE price > 100");
    auto result = db.execute(stmt);
    
    if (result) {
        std::cout << "Found " << result->row_count() << " products\n";
        
        for (int i = 0; i < result->row_count(); ++i) {
            int id = result->get_int(i, 0);
            std::string name = result->get_string(i, 1);
            double price = result->get_double(i, 2);
            
            std::cout << "ID: " << id << ", Name: " << name 
                      << ", Price: " << price << std::endl;
        }
    }
    
    return 0;
}
```

### Example 3: Error Handling

```cpp
#include <iostream>
#include "lyradb/database.h"
#include "lyradb/sql_parser.h"

int main() {
    auto parser = lyradb::SQLParser::create();
    
    auto stmt = parser->parse("INVALID SQL STATEMENT");
    
    if (!stmt) {
        std::cerr << "Parse error:\n" << parser->detailed_error() << std::endl;
        return 1;
    }
    
    return 0;
}
```

---

## Memory Management

The C++ API uses smart pointers for automatic memory management:

1. Use std::shared_ptr for parsers and statements
2. Use std::unique_ptr for query results
3. Rely on RAII for automatic cleanup
4. No manual delete necessary

Example:
```cpp
{
    auto parser = lyradb::SQLParser::create();  // Automatically cleaned up
    auto stmt = parser->parse(sql);              // Automatically cleaned up
    auto result = db.execute(stmt);              // Automatically cleaned up
}  // All resources released here
```

---

## Namespace

All classes are in the lyradb namespace:

```cpp
using namespace lyradb;

Database db;
auto parser = SQLParser::create();
```

Or use explicit namespace:

```cpp
lyradb::Database db;
auto parser = lyradb::SQLParser::create();
```

---

## Compilation

Include the necessary headers:

```cpp
#include "lyradb/database.h"
#include "lyradb/sql_parser.h"
#include "lyradb/query_result.h"
```

Compile with C++11 or later:

```bash
g++ -std=c++11 -I/path/to/lyradb/include program.cpp -o program -L/path/to/lyradb/lib -llyradb
```
