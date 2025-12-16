# LyraDB Documentation

Welcome to the LyraDB documentation. This comprehensive guide covers all aspects of using and developing with LyraDB.

## Documentation Index

### Getting Started

Start here if you are new to LyraDB.

1. Getting Started with LyraDB (01_GETTING_STARTED.md)
   - Introduction to LyraDB
   - System requirements
   - Installation overview
   - Quick start examples
   - Basic concepts

### Installation

Detailed installation instructions for different platforms.

2. Installation Guide (07_INSTALLATION.md)
   - System requirements
   - Windows installation
   - Linux installation
   - macOS installation
   - Docker installation
   - Build configuration

### SQL Guide

Complete reference for SQL statements and data types.

3. SQL Reference Guide (02_SQL_REFERENCE.md)
   - CREATE TABLE statement
   - INSERT statement
   - SELECT statement
   - SELECT with WHERE clause
   - UPDATE statement
   - DELETE statement
   - CREATE INDEX statement
   - DROP TABLE statement
   - DROP INDEX statement
   - Data type details
   - SQL best practices
   - Query execution model

4. Data Types Reference (05_DATA_TYPES_REFERENCE.md)
   - Data type overview
   - INT32 type
   - INT64 type
   - FLOAT32 type
   - FLOAT64 type
   - STRING type
   - BOOL type
   - Type conversion rules
   - Choosing appropriate types
   - Storage considerations

### API References

Programming guides for C and C++ APIs.

5. C API Reference (03_C_API_REFERENCE.md)
   - Core functions
   - Data types (lyradb_handle_t, lyradb_result_t)
   - Usage examples
   - Memory management
   - Error handling
   - Thread safety
   - Limitations

6. C++ API Reference (04_CPP_API_REFERENCE.md)
   - Database class
   - SQLParser class
   - QueryResult class
   - EngineQueryResult class
   - Statement classes
   - Usage examples
   - Memory management
   - Namespace usage

### Integration

Instructions for integrating LyraDB into your projects.

8. Integration Guide (08_INTEGRATION_GUIDE.md)
   - Quick 3-step integration
   - CMake configuration
   - Cross-platform setup
   - C and C++ examples
   - Prebuilt binaries usage
   - Compiler-specific instructions
   - Best practices

### Persistence

Working with database files and persistence.

9. Database File Format (09_DATABASE_FILE_FORMAT.md)
   - Using .db files (like SQLite)
   - Creating and saving databases
   - Loading databases from files
   - File format specification
   - API reference
   - Best practices
   - Backup and recovery

### Troubleshooting

Solutions for common issues.

7. Troubleshooting Guide (06_TROUBLESHOOTING.md)
   - Build issues
   - SQL syntax errors
   - Runtime errors
   - Type conversion issues
   - Performance issues
   - API usage issues
   - Platform-specific issues
   - Getting help

---

## Quick Navigation

By Topic:

Installation:
- Getting Started (Installation section)
- Installation Guide (complete guide)

Learning SQL:
- Getting Started (basic concepts)
- SQL Reference Guide (comprehensive)

Data Types:
- SQL Reference Guide (overview)
- Data Types Reference (detailed)

Using C API:
- Getting Started (C API example)
- C API Reference (complete reference)

Using C++ API:
- Getting Started (C++ API example)
- C++ API Reference (complete reference)

Solving Problems:
- Troubleshooting Guide (problem solutions)

Working with Files:
- Database File Format (.db files, save/load)
- Getting Started (persistence overview)

---

## Documentation Structure

Each document is self-contained but references related documents:

Getting Started
  ├─ SQL Reference
  ├─ C API Reference
  ├─ C++ API Reference
  └─ Installation Guide

SQL Reference
  ├─ Data Types Reference
  └─ Troubleshooting

API References
  ├─ Getting Started
  └─ Troubleshooting

Installation
  ├─ Getting Started
  └─ Troubleshooting

---

## Key Concepts

Before using LyraDB, understand these concepts:

Tables: Collections of rows and columns with defined schema

Columns: Named data containers with specific types

Data Types: INT32, INT64, FLOAT32, FLOAT64, STRING, BOOL

SQL Statements: CREATE TABLE, INSERT, SELECT, UPDATE, DELETE, CREATE INDEX, DROP

Indexes: Optimization structures for faster queries

WHERE Clause: Filtering mechanism for queries

---

## Common Tasks

Creating a Database:
1. Read Getting Started
2. Follow Installation Guide for your platform
3. Review SQL Reference for CREATE TABLE syntax

Inserting Data:
1. Check SQL Reference for INSERT syntax
2. Verify data types match column definitions
3. Handle errors using error messages

Querying Data:
1. Use SELECT statement from SQL Reference
2. Apply WHERE clause to filter results
3. Access results using appropriate API

Updating Data:
1. Use UPDATE statement from SQL Reference
2. Specify columns to update
3. Use WHERE clause to limit scope

Deleting Data:
1. Use DELETE statement from SQL Reference
2. Use WHERE clause to limit deletion
3. Verify data before deletion (no recovery possible)

Creating Indexes:
1. Identify frequently queried columns
2. Use CREATE INDEX from SQL Reference
3. Monitor performance improvement

---

## Development Resources

Example Code:
Located in examples/ directory of source code

Test Code:
Located in tests/ directory of source code

Source Code:
Located in src/ directory for learning implementation

Header Files:
Located in include/ directory for API definitions

---

## Version Information

Current Version: LyraDB v1.2.0

Supported Platforms:
- Windows (7 or later)
- Linux (Ubuntu 16.04+, CentOS 7+, Debian 8+)
- macOS (10.10 or later)

Supported Compilers:
- GCC 4.8+
- Clang 3.3+
- MSVC 2015+

Build System: CMake 3.10+

---

## Feature Overview

Data Definition Language (DDL):
- CREATE TABLE: Define table schema
- CREATE INDEX: Create indexes for optimization
- DROP TABLE: Remove tables
- DROP INDEX: Remove indexes

Data Manipulation Language (DML):
- INSERT: Add rows to tables
- SELECT: Query data from tables
- UPDATE: Modify existing rows
- DELETE: Remove rows from tables

Query Features:
- WHERE clause for filtering
- LIKE operator for pattern matching
- Type-safe value access

Error Handling:
- Detailed error messages
- Line and column information
- Context-aware hints

---

## Getting Help

Consult Documentation:
1. Use the index above to find relevant section
2. Review examples in Getting Started
3. Check Troubleshooting for common issues

Examine Code:
1. Review example code in examples/ directory
2. Study test code in tests/ directory
3. Check header files for API details

Check Status:
1. Read ISSUES_FIXED.md for known issues
2. Review IMPLEMENTATION_STATUS.md for features
3. Check README.md for project overview

---

## Next Steps

1. Install LyraDB following Installation Guide
2. Read Getting Started for quick overview
3. Work through examples in Getting Started
4. Learn SQL from SQL Reference Guide
5. Choose API (C or C++) and read corresponding reference
6. Build your application
7. Consult Troubleshooting if issues arise

---

## Document Versions

01_GETTING_STARTED.md - v1.0
02_SQL_REFERENCE.md - v1.0
03_C_API_REFERENCE.md - v1.0
04_CPP_API_REFERENCE.md - v1.0
05_DATA_TYPES_REFERENCE.md - v1.0
06_TROUBLESHOOTING.md - v1.0
07_INSTALLATION.md - v1.0
08_INTEGRATION_GUIDE.md - v1.0
09_DATABASE_FILE_FORMAT.md - v1.0
INDEX.md - v1.0

Last Updated: December 12, 2025

For the latest documentation, consult the docs/ directory in the LyraDB repository.
