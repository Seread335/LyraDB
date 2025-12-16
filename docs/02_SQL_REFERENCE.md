# SQL Reference Guide

## Overview

This guide provides comprehensive documentation of all SQL statements supported by LyraDB.

## Statement Types

### CREATE TABLE

Creates a new table in the database.

Syntax:
```sql
CREATE TABLE table_name (
    column_name data_type,
    column_name data_type,
    ...
)
```

Parameters:

- table_name: The name of the table to create (alphanumeric and underscore)
- column_name: The name of each column
- data_type: The data type for each column (INT32, INT64, FLOAT32, FLOAT64, STRING, BOOL)

Example:
```sql
CREATE TABLE employees (
    id INT32,
    first_name STRING,
    last_name STRING,
    salary FLOAT64,
    hire_date STRING,
    is_active BOOL
)
```

Constraints:

- Table name must be unique
- Column names must be unique within the table
- All columns must have a specified data type
- At least one column is required

---

### INSERT

Inserts one or more rows into a table.

Syntax:
```sql
INSERT INTO table_name (column1, column2, ...) VALUES (value1, value2, ...)
INSERT INTO table_name VALUES (value1, value2, ...)
```

Parameters:

- table_name: The name of the target table
- column names: Optional list of columns to insert into
- values: Data values matching the specified columns or all columns

Examples:
```sql
INSERT INTO employees VALUES (1, 'John', 'Doe', 75000.00, '2020-01-15', 1)

INSERT INTO employees (id, first_name, last_name, salary) 
VALUES (2, 'Jane', 'Smith', 85000.00)
```

Constraints:

- Table must exist
- Number of values must match number of columns (if columns specified)
- Data types are automatically converted to strings internally
- NULL values are stored as empty strings

---

### SELECT

Retrieves data from one or more columns in a table.

Syntax:
```sql
SELECT column1, column2, ... FROM table_name
SELECT * FROM table_name
SELECT column1, column2 FROM table_name WHERE condition
```

Parameters:

- columns: List of columns to retrieve, or * for all columns
- table_name: The name of the source table
- WHERE clause: Optional filter condition

Examples:
```sql
SELECT * FROM employees

SELECT first_name, last_name, salary FROM employees

SELECT first_name, salary FROM employees WHERE salary > 80000
```

Constraints:

- Table must exist
- Column names must exist in the table
- WHERE condition is optional
- Results are returned in the order rows were inserted

---

### SELECT with WHERE Clause

Filters rows based on specified conditions.

Supported Operators:

- = (equal)
- != (not equal)
- < (less than)
- > (greater than)
- <= (less than or equal)
- >= (greater than or equal)
- LIKE (pattern matching)

Examples:
```sql
SELECT * FROM employees WHERE salary > 75000

SELECT * FROM employees WHERE is_active = 1

SELECT * FROM employees WHERE last_name LIKE 'Smith'

SELECT * FROM employees WHERE salary >= 80000 AND salary <= 100000
```

LIKE Pattern Matching:

- % matches zero or more characters
- _ matches exactly one character

Examples:
```sql
WHERE first_name LIKE 'J%'      -- Names starting with J
WHERE first_name LIKE '%n'      -- Names ending with n
WHERE first_name LIKE '_ohn'    -- 4-character names ending with ohn
```

---

### UPDATE

Modifies existing rows in a table.

Syntax:
```sql
UPDATE table_name SET column1 = value1, column2 = value2 WHERE condition
UPDATE table_name SET column1 = value1
```

Parameters:

- table_name: The name of the target table
- column = value: Column assignments
- WHERE clause: Optional filter condition

Examples:
```sql
UPDATE employees SET salary = 95000 WHERE id = 1

UPDATE employees SET is_active = 0 WHERE salary < 50000

UPDATE employees SET salary = 80000
```

Constraints:

- Table must exist
- Column names must exist
- WHERE clause is optional (all rows updated if omitted)
- Specified columns are updated to new values

---

### DELETE

Removes rows from a table.

Syntax:
```sql
DELETE FROM table_name WHERE condition
DELETE FROM table_name
```

Parameters:

- table_name: The name of the target table
- WHERE clause: Optional filter condition

Examples:
```sql
DELETE FROM employees WHERE id = 1

DELETE FROM employees WHERE salary < 50000

DELETE FROM employees
```

Constraints:

- Table must exist
- WHERE clause is optional (all rows deleted if omitted)
- Deleted rows cannot be recovered

---

### CREATE INDEX

Creates an index on one or more columns for query optimization.

Syntax:
```sql
CREATE INDEX index_name ON table_name (column1, column2, ...)
CREATE INDEX index_name ON table_name (column_name)
```

Parameters:

- index_name: The name of the index (must be unique)
- table_name: The name of the target table
- column_name(s): The column(s) to index

Examples:
```sql
CREATE INDEX idx_salary ON employees (salary)

CREATE INDEX idx_name ON employees (last_name, first_name)
```

Constraints:

- Table must exist
- Column names must exist in the table
- Index name must be unique
- At least one column must be specified

---

### DROP TABLE

Removes a table from the database.

Syntax:
```sql
DROP TABLE table_name
DROP TABLE IF EXISTS table_name
```

Parameters:

- table_name: The name of the table to drop
- IF EXISTS: Optional clause to prevent error if table doesn't exist

Examples:
```sql
DROP TABLE employees

DROP TABLE IF EXISTS temp_data
```

Constraints:

- Table must exist (unless IF EXISTS is used)
- All data in the table is permanently deleted

---

### DROP INDEX

Removes an index from the database.

Syntax:
```sql
DROP INDEX index_name
DROP INDEX IF EXISTS index_name
```

Parameters:

- index_name: The name of the index to drop
- IF EXISTS: Optional clause to prevent error if index doesn't exist

Examples:
```sql
DROP INDEX idx_salary

DROP INDEX IF EXISTS idx_temp
```

Constraints:

- Index must exist (unless IF EXISTS is used)

---

## Data Type Details

### INT32

Stores 32-bit signed integers.

Range: -2,147,483,648 to 2,147,483,647

Example:
```sql
CREATE TABLE sales (id INT32, quantity INT32)
INSERT INTO sales VALUES (1, 100)
```

### INT64

Stores 64-bit signed integers.

Range: -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807

Example:
```sql
CREATE TABLE analytics (event_id INT64, timestamp INT64)
```

### FLOAT32

Stores 32-bit floating-point numbers.

Precision: Approximately 7 decimal digits

Example:
```sql
CREATE TABLE measurements (id INT32, value FLOAT32)
```

### FLOAT64

Stores 64-bit floating-point numbers.

Precision: Approximately 15 decimal digits

Example:
```sql
CREATE TABLE prices (id INT32, price FLOAT64)
INSERT INTO prices VALUES (1, 99.99)
```

### STRING

Stores variable-length text data.

Maximum Length: No predetermined limit (limited by available memory)

Example:
```sql
CREATE TABLE users (id INT32, name STRING, email STRING)
INSERT INTO users VALUES (1, 'John Doe', 'john@example.com')
```

### BOOL

Stores boolean values (true or false).

Representation: Stored as integer (0 for false, 1 for true)

Example:
```sql
CREATE TABLE features (id INT32, enabled BOOL)
INSERT INTO features VALUES (1, 1)
```

---

## SQL Best Practices

1. Use meaningful table and column names
2. Specify column names in INSERT statements for clarity
3. Use WHERE clauses to filter unnecessary data
4. Create indexes on frequently queried columns
5. Delete or archive old data periodically
6. Use appropriate data types for your data
7. Validate input data before inserting
8. Use transactions for data consistency (when available)

## Limitations

1. No JOIN operations between tables
2. No aggregate functions (COUNT, SUM, AVG, etc.)
3. No subqueries
4. No GROUP BY or ORDER BY clauses
5. No transactions or ACID properties
6. No foreign key constraints
7. Single-threaded operation

## Query Execution Model

1. SQL statement is parsed into an abstract syntax tree
2. Parser validates statement syntax
3. Database engine locates the target table
4. Engine executes the statement operation
5. Results are returned to the caller

---

## Error Handling

If a SQL statement contains errors, LyraDB returns detailed error information:

- Error message describing the problem
- Line number of the error
- Column number where the error was detected
- The token that caused the error
- A helpful hint for correction

Example:
```
SQL Syntax Error:
  Message: Unexpected token
  Location: Line 1, Column 25
  Token: 'INVALID'
  Hint: Expected data type (INT32, INT64, FLOAT32, FLOAT64, STRING, BOOL)
```
