# Data Types Reference

## Overview

LyraDB supports six fundamental data types for storing different kinds of information in tables. Each data type has specific ranges, precision, and use cases.

## Data Type Summary

Data Type | Bytes | Range | Precision | Best Use Case
-----------|-------|-------|-----------|---------------
INT32 | 4 | -2.1B to 2.1B | Exact integers | IDs, counts, quantities
INT64 | 8 | -9.2E18 to 9.2E18 | Large integers | Timestamps, large IDs
FLOAT32 | 4 | ±10^38 | 7 digits | Measurements, scientific data
FLOAT64 | 8 | ±10^308 | 15 digits | Financial, scientific data
STRING | Variable | N/A | N/A | Text, names, descriptions
BOOL | 1 | 0 or 1 | Exact boolean | Flags, status

---

## Detailed Type Specifications

### INT32

32-bit signed integer type.

Range: -2,147,483,648 to 2,147,483,647

Example Declaration:
```sql
CREATE TABLE products (
    id INT32,
    quantity INT32,
    warehouse_id INT32
)
```

Example Usage:
```sql
INSERT INTO products VALUES (1, 100, 5)
SELECT * FROM products WHERE quantity > 50
UPDATE products SET quantity = 150 WHERE id = 1
```

Characteristics:

- Fixed size of 4 bytes
- Supports arithmetic operations
- Suitable for most integer values in business applications
- Overflow occurs if value exceeds range

---

### INT64

64-bit signed integer type for large integers and timestamps.

Range: -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807

Example Declaration:
```sql
CREATE TABLE events (
    event_id INT64,
    timestamp INT64,
    user_id INT64
)
```

Example Usage:
```sql
INSERT INTO events VALUES (9223372036854775807, 1702425600, 123456789)
SELECT * FROM events WHERE timestamp > 1702425600
```

Characteristics:

- Fixed size of 8 bytes
- Suitable for timestamps in milliseconds
- Supports very large ID numbers
- Backward compatible with INT32 values

---

### FLOAT32

32-bit floating-point type for approximate decimal values.

Precision: Approximately 7 significant decimal digits

Range: ±1.4E-45 to ±3.4E+38

Example Declaration:
```sql
CREATE TABLE measurements (
    sensor_id INT32,
    temperature FLOAT32,
    humidity FLOAT32
)
```

Example Usage:
```sql
INSERT INTO measurements VALUES (1, 23.5, 45.2)
SELECT * FROM measurements WHERE temperature > 20.0
```

Characteristics:

- Fixed size of 4 bytes
- IEEE 754 standard format
- Approximate representation (rounding errors possible)
- Suitable for non-critical decimal values
- Faster computation than FLOAT64

Precision Example:

```
Value: 1.23456789
Stored: 1.234568 (7 significant digits)
Error: 0.00000089
```

---

### FLOAT64

64-bit floating-point type for high-precision decimal values.

Precision: Approximately 15-17 significant decimal digits

Range: ±2.2E-308 to ±1.8E+308

Example Declaration:
```sql
CREATE TABLE financial (
    transaction_id INT32,
    amount FLOAT64,
    exchange_rate FLOAT64
)
```

Example Usage:
```sql
INSERT INTO financial VALUES (1, 1234.56, 0.95)
SELECT * FROM financial WHERE amount > 1000.00
```

Characteristics:

- Fixed size of 8 bytes
- IEEE 754 standard format
- Higher precision than FLOAT32
- Suitable for financial calculations
- Recommended for scientific data
- More precise but slower than FLOAT32

Precision Example:

```
Value: 3.14159265358979
Stored: 3.14159265358979 (15+ significant digits)
Error: Negligible
```

---

### STRING

Variable-length text type for storing character data.

Maximum Length: Limited only by available memory

Example Declaration:
```sql
CREATE TABLE users (
    id INT32,
    name STRING,
    email STRING,
    address STRING,
    description STRING
)
```

Example Usage:
```sql
INSERT INTO users VALUES (1, 'John Doe', 'john@example.com', '123 Main St', 'Customer')
SELECT * FROM users WHERE name LIKE 'John%'
UPDATE users SET email = 'newemail@example.com' WHERE id = 1
```

Characteristics:

- Variable length based on content
- Supports Unicode characters
- Supports Vietnamese and international text
- No predetermined size limit
- Automatically trimmed of unnecessary whitespace
- Suitable for names, emails, descriptions

Encoding:

- UTF-8 encoding supported
- International characters supported
- Escape sequences: none required

Pattern Matching (LIKE):

- % matches zero or more characters
- _ matches exactly one character

Examples:
```sql
WHERE name LIKE 'A%'        -- Starts with A
WHERE name LIKE '%son'      -- Ends with son
WHERE name LIKE 'Jo%n'      -- Starts with Jo, ends with n
WHERE email LIKE '%@%.%'    -- Contains @ and .
```

---

### BOOL

Boolean type for true/false values.

Representation: 0 for false, 1 for true

Example Declaration:
```sql
CREATE TABLE features (
    feature_id INT32,
    enabled BOOL,
    is_premium BOOL,
    verified BOOL
)
```

Example Usage:
```sql
INSERT INTO features VALUES (1, 1, 0, 1)
SELECT * FROM features WHERE enabled = 1
UPDATE features SET verified = 1 WHERE feature_id = 5
```

Characteristics:

- Stored as integer (0 or 1)
- Size: 1 byte
- Suitable for boolean flags and status
- Used in conditionals and WHERE clauses

Value Interpretation:

- 0 or false: Represents false condition
- 1 or true: Represents true condition
- Non-zero values: Interpreted as true
- NULL: Not supported (use 0 for false)

---

## Type Conversion Rules

When inserting values of different types, LyraDB applies automatic conversion:

### String Conversion

All values are internally converted to strings:

```sql
INSERT INTO mixed_table VALUES (123, 45.67, 'text', 1)
```

All values are stored as: "123", "45.67", "text", "1"

### Retrieval Conversion

When retrieving values, use appropriate accessor methods:

C++ API:
```cpp
int id = result->get_int(0, 0);        // String to INT
double price = result->get_double(0, 1); // String to FLOAT64
std::string text = result->get_string(0, 2); // String
bool flag = result->get_bool(0, 3);    // String to BOOL
```

### Conversion Rules

Source | Target | Result
--------|--------|--------
"123" | INT32 | 123
"123.45" | INT32 | 123 (decimal truncated)
"123.45" | FLOAT64 | 123.45
"true" | BOOL | 0 (invalid, false)
"1" | BOOL | 1 (true)
"invalid" | INT32 | 0 (conversion error)
123 | STRING | "123"
45.67 | STRING | "45.67"

---

## Choosing Data Types

### For Identifiers

Use INT32 for most IDs:
```sql
CREATE TABLE products (
    id INT32,
    category_id INT32
)
```

Use INT64 for very large ID spaces or timestamps:
```sql
CREATE TABLE events (
    id INT64,
    timestamp INT64
)
```

### For Numeric Values

Use INT32 or INT64 for whole numbers:
```sql
INSERT INTO inventory VALUES (100, 5, 25)  -- quantity, warehouse, stock
```

Use FLOAT64 for monetary values:
```sql
INSERT INTO prices VALUES (99.99, 0.05)    -- price, tax_rate
```

Use FLOAT32 for non-critical measurements:
```sql
INSERT INTO sensors VALUES (23.5, 45.2)    -- temperature, humidity
```

### For Text Data

Always use STRING for text:
```sql
INSERT INTO users VALUES ('John Doe', 'john@example.com', 'Developer')
```

STRING supports:
- Names and addresses
- Email and phone
- Descriptions and comments
- JSON-like data
- International text

### For Status and Flags

Use BOOL or INT32:
```sql
INSERT INTO features VALUES (1, 1, 1)  -- enabled, verified, premium
```

Or with BOOL type:
```sql
CREATE TABLE flags (
    feature_id INT32,
    is_active BOOL,
    is_verified BOOL
)
```

---

## Type Compatibility in Operations

### Comparison Operations

Numeric types can be compared:
```sql
WHERE price > 100.00      -- FLOAT64 comparison
WHERE quantity > 50       -- INT32 comparison
WHERE timestamp > 1700000000  -- INT64 comparison
```

STRING comparisons:
```sql
WHERE name = 'John'
WHERE email LIKE '%@example.com%'
```

BOOL comparisons:
```sql
WHERE is_active = 1
WHERE verified = 0
```

### Arithmetic Operations

Supported on numeric types in internal calculations (INT32, INT64, FLOAT32, FLOAT64)

Not supported:
- STRING arithmetic
- BOOL arithmetic
- Cross-type arithmetic

---

## Common Data Type Patterns

### User Record
```sql
CREATE TABLE users (
    id INT32,
    username STRING,
    email STRING,
    age INT32,
    is_verified BOOL
)
```

### Financial Transaction
```sql
CREATE TABLE transactions (
    id INT64,
    account_id INT32,
    amount FLOAT64,
    timestamp INT64,
    is_approved BOOL
)
```

### Product Catalog
```sql
CREATE TABLE products (
    id INT32,
    name STRING,
    price FLOAT64,
    quantity INT32,
    description STRING
)
```

### Sensor Data
```sql
CREATE TABLE measurements (
    sensor_id INT32,
    timestamp INT64,
    temperature FLOAT32,
    humidity FLOAT32
)
```

---

## Performance Considerations

1. INT32 is fastest for integer operations
2. FLOAT64 has higher precision but slightly slower
3. STRING operations depend on length
4. BOOL operations are very fast
5. Use appropriate type for your data to minimize storage

---

## Storage Notes

LyraDB stores all values as strings internally. The actual byte sizes listed above refer to the data types' logical definitions. Storage efficiency depends on string representation length.

Example:
- INT32 value 1,234,567 is stored as string "1234567" (7 bytes)
- FLOAT64 value 1234567.89 is stored as string "1234567.89" (10 bytes)
- STRING value "Hello World" is stored as is (11 bytes)
