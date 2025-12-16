# C API Reference

## Overview

The C API provides a simple interface to interact with LyraDB from C programs. All functions are declared in the lyradb_c.h header file.

## Core Functions

### lyradb_create_database

Creates a new in-memory database instance.

Signature:
```c
lyradb_handle_t lyradb_create_database();
```

Return Value:

- lyradb_handle_t: Handle to the created database

Description:

Creates and initializes a new database in memory. The returned handle must be used for all subsequent database operations. The database is empty and ready for table creation.

Example:
```c
lyradb_handle_t db = lyradb_create_database();
if (db) {
    printf("Database created successfully\n");
}
```

---

### lyradb_destroy_database

Destroys a database instance and releases all resources.

Signature:
```c
void lyradb_destroy_database(lyradb_handle_t db);
```

Parameters:

- db: Database handle returned from lyradb_create_database

Description:

Closes the database and frees all allocated memory. After calling this function, the database handle is no longer valid and should not be used.

Example:
```c
lyradb_destroy_database(db);
db = NULL;
```

---

### lyradb_execute

Executes a SQL statement against the database.

Signature:
```c
lyradb_result_t lyradb_execute(lyradb_handle_t db, const char* sql);
```

Parameters:

- db: Database handle returned from lyradb_create_database
- sql: SQL statement string to execute

Return Value:

- lyradb_result_t: Result structure containing status and data

Description:

Parses and executes a SQL statement. The result structure contains the success status and any error message if the operation failed.

Example:
```c
const char* sql = "CREATE TABLE users (id INT32, name STRING)";
lyradb_result_t result = lyradb_execute(db, sql);

if (!result.success) {
    printf("Error: %s\n", result.error_message);
}
free(result.error_message);
```

---

## Data Types

### lyradb_handle_t

Opaque handle representing a database instance.

```c
typedef void* lyradb_handle_t;
```

### lyradb_result_t

Structure containing the result of a SQL operation.

```c
typedef struct {
    int success;              /* 1 if successful, 0 if error */
    const char* error_message;/* NULL if successful, error text otherwise */
    void* data;               /* Result data (currently unused) */
} lyradb_result_t;
```

Members:

- success: Non-zero if the operation succeeded, 0 if it failed
- error_message: Points to error message string if operation failed
- data: Reserved for future use

---

## Usage Examples

### Example 1: Create a Table and Insert Data

```c
#include <stdio.h>
#include <stdlib.h>
#include "lyradb_c.h"

int main() {
    lyradb_handle_t db = lyradb_create_database();
    
    if (!db) {
        fprintf(stderr, "Failed to create database\n");
        return 1;
    }
    
    // Create table
    lyradb_result_t result = lyradb_execute(db,
        "CREATE TABLE products (id INT32, name STRING, price FLOAT64)");
    
    if (!result.success) {
        fprintf(stderr, "Create table error: %s\n", result.error_message);
        free(result.error_message);
        lyradb_destroy_database(db);
        return 1;
    }
    free(result.error_message);
    
    // Insert data
    result = lyradb_execute(db,
        "INSERT INTO products VALUES (1, 'Laptop', 999.99)");
    
    if (!result.success) {
        fprintf(stderr, "Insert error: %s\n", result.error_message);
        free(result.error_message);
    } else {
        printf("Data inserted successfully\n");
    }
    free(result.error_message);
    
    lyradb_destroy_database(db);
    return 0;
}
```

### Example 2: Query Data

```c
#include <stdio.h>
#include "lyradb_c.h"

int main() {
    lyradb_handle_t db = lyradb_create_database();
    
    // Create and populate table
    lyradb_execute(db,
        "CREATE TABLE employees (id INT32, name STRING, salary FLOAT64)");
    
    lyradb_execute(db,
        "INSERT INTO employees VALUES (1, 'Alice', 75000)");
    lyradb_execute(db,
        "INSERT INTO employees VALUES (2, 'Bob', 85000)");
    
    // Query data
    lyradb_result_t result = lyradb_execute(db,
        "SELECT * FROM employees WHERE salary > 80000");
    
    if (!result.success) {
        fprintf(stderr, "Query error: %s\n", result.error_message);
    } else {
        printf("Query executed successfully\n");
    }
    
    free(result.error_message);
    lyradb_destroy_database(db);
    return 0;
}
```

### Example 3: Update and Delete

```c
#include <stdio.h>
#include "lyradb_c.h"

int main() {
    lyradb_handle_t db = lyradb_create_database();
    
    // Create table and insert data
    lyradb_execute(db,
        "CREATE TABLE inventory (id INT32, item STRING, quantity INT32)");
    
    lyradb_execute(db,
        "INSERT INTO inventory VALUES (1, 'Widget', 100)");
    
    // Update data
    lyradb_result_t result = lyradb_execute(db,
        "UPDATE inventory SET quantity = 50 WHERE id = 1");
    
    if (!result.success) {
        fprintf(stderr, "Update error: %s\n", result.error_message);
    }
    free(result.error_message);
    
    // Delete data
    result = lyradb_execute(db,
        "DELETE FROM inventory WHERE quantity < 10");
    
    if (!result.success) {
        fprintf(stderr, "Delete error: %s\n", result.error_message);
    }
    free(result.error_message);
    
    lyradb_destroy_database(db);
    return 0;
}
```

---

## Memory Management

The C API uses dynamic memory allocation for error messages. Follow these guidelines:

1. Always free error messages after use:

```c
free(result.error_message);
```

2. Do not reuse pointers after calling lyradb_destroy_database

3. Do not access database handle after destruction

4. Check for NULL returns before using database handles

---

## Error Handling

All C API functions return status information. Always check the result:

```c
lyradb_result_t result = lyradb_execute(db, sql);

if (!result.success) {
    // Handle error
    fprintf(stderr, "Error: %s\n", result.error_message);
    free(result.error_message);
    return -1;
}

// Continue with successful operation
free(result.error_message);
```

---

## Thread Safety

LyraDB is not thread-safe. Do not access the same database handle from multiple threads simultaneously. If your application requires multi-threaded access, implement external synchronization.

---

## Limitations

1. Single database instance per handle
2. No prepared statements
3. No result set iteration (results are not returned)
4. No transaction support
5. Limited error information in result structure

---

## Header File

The C API is declared in include/lyradb_c.h. Include this header in your C programs:

```c
#include "lyradb_c.h"
```

Compile and link against the LyraDB library:

```bash
gcc -I/path/to/lyradb/include program.c -o program -L/path/to/lyradb/lib -llyradb_c
```
