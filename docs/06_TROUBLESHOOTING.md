# Troubleshooting Guide

## Overview

This guide provides solutions for common issues encountered when using LyraDB.

---

## Build Issues

### CMake Not Found

Problem: CMake is not installed or not in system PATH.

Solution:

1. Install CMake from https://cmake.org/download/
2. Add CMake installation directory to system PATH
3. Verify installation: `cmake --version`
4. Retry build process

### Compiler Not Found

Problem: C++ compiler is not installed or not in PATH.

Solution on Windows:

1. Install Microsoft Visual Studio 2015 or later
2. Or install Visual Studio Build Tools
3. Ensure Visual C++ components are selected during installation

Solution on Linux:

1. Install build tools: `sudo apt-get install build-essential`
2. Or use specific compiler: `sudo apt-get install g++`

Solution on macOS:

1. Install Xcode: `xcode-select --install`
2. Or install Xcode from App Store

### CMake Configuration Error

Problem: CMake fails with configuration error.

Solution:

1. Delete the build directory: `rm -rf build`
2. Recreate build directory: `mkdir build`
3. Re-run CMake: `cmake .. -DCMAKE_BUILD_TYPE=Release`
4. Check for specific error messages
5. Verify all dependencies are installed

### Build Fails with Linker Error

Problem: Linking fails when building project.

Solution:

1. Clean build: `cmake --build . --target clean`
2. Rebuild from scratch: `cmake --build . --config Release`
3. Verify all source files are present
4. Check for typos in library names
5. Ensure library paths are correct

---

## SQL Syntax Issues

### Unexpected Token Error

Problem: Parse error stating "Unexpected token at line X, column Y"

Causes and Solutions:

1. Typo in keyword:
   - Check SQL keywords are spelled correctly
   - Keywords are case-insensitive but should match documentation

2. Missing comma in CREATE TABLE:
   ```sql
   -- Wrong
   CREATE TABLE users (id INT32 name STRING)
   
   -- Correct
   CREATE TABLE users (id INT32, name STRING)
   ```

3. Invalid data type:
   ```sql
   -- Wrong
   CREATE TABLE users (id INTEGER)
   
   -- Correct (use INT32, INT64, FLOAT32, FLOAT64, STRING, or BOOL)
   CREATE TABLE users (id INT32)
   ```

4. Mismatched parentheses:
   ```sql
   -- Wrong
   INSERT INTO users (id, name VALUES (1, 'John')
   
   -- Correct
   INSERT INTO users (id, name) VALUES (1, 'John')
   ```

### Missing Required Clause

Problem: "Expected X but found Y"

Example solutions:

1. CREATE TABLE without columns:
   ```sql
   -- Wrong
   CREATE TABLE users
   
   -- Correct
   CREATE TABLE users (id INT32, name STRING)
   ```

2. INSERT without table name:
   ```sql
   -- Wrong
   INSERT VALUES (1, 'John')
   
   -- Correct
   INSERT INTO users VALUES (1, 'John')
   ```

3. SELECT without FROM clause:
   ```sql
   -- Wrong (depends on implementation)
   SELECT id, name
   
   -- Correct
   SELECT id, name FROM users
   ```

### Invalid Table or Column Name

Problem: Table or column does not exist.

Diagnosis:

1. Verify table exists: Check that CREATE TABLE was executed successfully
2. Check spelling: SQL names are case-sensitive
3. Ensure table was not dropped: Previous DROP statement removed it

Solution:

```sql
-- List tables by examining recent CREATE statements
-- Check error message for exact name being searched
-- Create table if missing: CREATE TABLE ...
```

### Invalid WHERE Clause

Problem: WHERE condition syntax error.

Examples:

1. Missing condition value:
   ```sql
   -- Wrong
   SELECT * FROM users WHERE id >
   
   -- Correct
   SELECT * FROM users WHERE id > 5
   ```

2. Invalid operator:
   ```sql
   -- Wrong
   SELECT * FROM users WHERE id <> 5  (not supported)
   
   -- Correct (use !=)
   SELECT * FROM users WHERE id != 5
   ```

3. LIKE pattern without quotes:
   ```sql
   -- Wrong
   SELECT * FROM users WHERE name LIKE %John%
   
   -- Correct
   SELECT * FROM users WHERE name LIKE '%John%'
   ```

---

## Runtime Issues

### Database Fails to Open

Problem: Cannot create or open database.

Possible Causes and Solutions:

1. Insufficient permissions:
   - Check directory write permissions
   - Run with elevated privileges if necessary

2. Out of memory:
   - Close other applications
   - Reduce data size
   - Add more RAM

3. Database file corruption:
   - Delete corrupted database files
   - Start fresh with new database
   - Restore from backup

### Data Not Appearing After Insert

Problem: INSERT appears successful but SELECT returns no data.

Diagnosis Steps:

1. Verify INSERT succeeded (check return status)
2. Verify SELECT targets correct table
3. Check for filtering conditions (WHERE clause)

Solution:

```cpp
// Verify INSERT success
lyradb_result_t result = lyradb_execute(db, insert_sql);
if (!result.success) {
    printf("Insert failed: %s\n", result.error_message);
}

// Verify data with simple SELECT
result = lyradb_execute(db, "SELECT * FROM table");
if (!result.success) {
    printf("Select failed: %s\n", result.error_message);
}
```

### NULL or Empty Values

Problem: Values appear NULL or empty in results.

Causes:

1. Data not inserted into column
2. Incorrect column index in result access
3. Type conversion failed

Solution:

```cpp
// Verify row and column indices
if (row_index < result->row_count() && col_index < result->column_count()) {
    std::string value = result->get_value(row_index, col_index);
    if (value.empty()) {
        printf("Value is empty or NULL\n");
    }
}
```

### Memory Leak Warnings

Problem: Valgrind or other tools report memory leaks.

Solution:

1. Ensure all database handles are properly destroyed:
   ```c
   lyradb_destroy_database(db);
   ```

2. Free error messages:
   ```c
   free(result.error_message);
   ```

3. Use C++ RAII patterns:
   ```cpp
   {
       auto parser = lyradb::SQLParser::create();
       // Usage
   }  // Automatically cleaned up
   ```

---

## Type Conversion Issues

### Unexpected Type Conversion Result

Problem: get_int() returns 0 for non-empty value.

Diagnosis:

1. Value might not be a valid integer
2. Value might be empty string
3. Overflow or underflow occurred

Solution:

```cpp
std::string value = result->get_value(row, col);

// Check for empty string
if (value.empty()) {
    printf("Value is empty\n");
    return;
}

// Attempt conversion with error checking
try {
    int converted = std::stoi(value);
    printf("Converted: %d\n", converted);
} catch (std::invalid_argument& e) {
    printf("Invalid integer: %s\n", value.c_str());
} catch (std::out_of_range& e) {
    printf("Integer out of range: %s\n", value.c_str());
}
```

### Float Precision Issues

Problem: FLOAT values show unexpected decimal places.

Cause: Floating-point representation is approximate.

Example:
```
Expected: 0.1
Actual:   0.10000000000000001
```

Solution:

1. Use FLOAT64 for financial data requiring precision
2. Round results to acceptable precision
3. Use string representation for display

---

## Performance Issues

### Slow Query Performance

Problem: Queries run slowly with large datasets.

Solutions:

1. Create indexes on frequently searched columns:
   ```sql
   CREATE INDEX idx_user_name ON users (name)
   ```

2. Use WHERE clauses to filter data:
   ```sql
   SELECT * FROM users WHERE age > 18  -- faster than SELECT *
   ```

3. Select specific columns:
   ```sql
   SELECT name, email FROM users  -- faster than SELECT *
   ```

4. Delete old or unnecessary data

### High Memory Usage

Problem: Application consumes excessive memory.

Solutions:

1. Process data in smaller batches
2. Delete old records periodically
3. Reduce table sizes
4. Close unused database connections
5. Profile memory usage with tools like Valgrind or profiler

### Large Result Sets

Problem: Query returns very large number of rows.

Solutions:

1. Add WHERE clause to filter results
2. Process results in smaller batches
3. Use indexes to speed up filtering
4. Archive old data to separate database

---

## API Usage Issues

### C API: Error Messages Not Freed

Problem: Memory leak from unfreed error messages.

Solution:

```c
lyradb_result_t result = lyradb_execute(db, sql);
if (!result.success) {
    printf("Error: %s\n", result.error_message);
}
free(result.error_message);  // Always free
```

### C++ API: Null Pointer Exception

Problem: Accessing null parser or result.

Solution:

```cpp
auto parser = lyradb::SQLParser::create();
if (!parser) {
    printf("Failed to create parser\n");
    return;
}

auto stmt = parser->parse(sql);
if (!stmt) {
    printf("Parse error: %s\n", parser->error().c_str());
    return;
}

auto result = db.execute(stmt);
if (!result) {
    printf("Execution failed\n");
    return;
}
```

### C++ API: Invalid Cast

Problem: Attempting to use parser or result after destruction.

Solution: Use smart pointers correctly:

```cpp
{
    auto parser = lyradb::SQLParser::create();
    auto stmt = parser->parse(sql);
    auto result = db.execute(stmt);
    // All objects valid here
}
// Objects destroyed here - do not use

// Correct: keep objects in scope
auto parser = lyradb::SQLParser::create();
auto stmt = parser->parse(sql);
auto result = db.execute(stmt);
// Use objects here, they remain valid
```

---

## Platform-Specific Issues

### Windows Build Issues

1. Visual Studio not installed:
   - Install Visual Studio Community or Build Tools
   - Include C++ development tools

2. Path issues with spaces:
   - Use quotes: `"C:\Program Files\..."`
   - Or use short paths

### Linux Build Issues

1. Missing development headers:
   ```bash
   sudo apt-get install build-essential
   sudo apt-get install cmake
   ```

2. Library not found:
   ```bash
   ldconfig -p | grep lyradb
   export LD_LIBRARY_PATH=/path/to/lib:$LD_LIBRARY_PATH
   ```

### macOS Build Issues

1. Xcode not installed:
   ```bash
   xcode-select --install
   ```

2. Homebrew packages needed:
   ```bash
   brew install cmake
   brew install gcc
   ```

---

## Getting Help

If you encounter issues not covered here:

1. Check the ISSUES_FIXED.md file for known problems
2. Review the API reference documentation
3. Examine example code for correct usage patterns
4. Check error messages for detailed information
5. Verify SQL syntax against SQL Reference guide

---

## Reporting Issues

When reporting a problem, provide:

1. LyraDB version (v1.2.0 or check header file)
2. Operating system and version
3. Compiler and version
4. Exact SQL statement that causes problem
5. Error message (complete, with line numbers)
6. Steps to reproduce
7. Expected vs. actual behavior
