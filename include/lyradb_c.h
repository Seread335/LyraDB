#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* Opaque type handles for C API */
typedef void* lyra_db_t;
typedef void* lyra_result_t;
typedef void* lyra_stmt_t;

/* Error codes */
typedef enum {
    LYRA_OK = 0,
    LYRA_ERROR = 1,
    LYRA_NOT_FOUND = 2,
    LYRA_CONSTRAINT = 3,
    LYRA_MEMORY = 4,
    LYRA_INVALID_SQL = 5,
    LYRA_IO_ERROR = 6
} lyra_errcode_t;

/* Data types */
typedef enum {
    LYRA_TYPE_INT32 = 0,
    LYRA_TYPE_INT64 = 1,
    LYRA_TYPE_FLOAT32 = 2,
    LYRA_TYPE_FLOAT64 = 3,
    LYRA_TYPE_STRING = 4,
    LYRA_TYPE_BOOL = 5,
    LYRA_TYPE_NULL = 6
} lyra_datatype_t;

/**
 * DATABASE MANAGEMENT
 */

/**
 * Open or create a database
 * 
 * @param path Path to database file
 * @param errmsg Pointer to error message (set by function)
 * @return Database handle, or NULL on error
 */
lyra_db_t lyra_open(const char* path, char** errmsg);

/**
 * Close database and free resources
 * 
 * @param db Database handle
 * @return LYRA_OK on success
 */
lyra_errcode_t lyra_close(lyra_db_t db);

/**
 * Get last error message
 * 
 * @param db Database handle
 * @return Error message string
 */
const char* lyra_errmsg(lyra_db_t db);

/**
 * Get database file size in bytes
 * 
 * @param db Database handle
 * @return File size, or -1 on error
 */
int64_t lyra_filesize(lyra_db_t db);

/**
 * Compact database (remove unused space)
 * 
 * @param db Database handle
 * @return LYRA_OK on success
 */
lyra_errcode_t lyra_compact(lyra_db_t db);

/**
 * QUERY EXECUTION
 */

/**
 * Execute SQL query
 * 
 * @param db Database handle
 * @param sql SQL query string
 * @param errmsg Pointer to error message
 * @return Result handle, or NULL on error
 */
lyra_result_t lyra_query(lyra_db_t db, const char* sql, char** errmsg);

/**
 * Get number of rows in result
 * 
 * @param result Result handle
 * @return Row count
 */
int64_t lyra_rows(lyra_result_t result);

/**
 * Get number of columns in result
 * 
 * @param result Result handle
 * @return Column count
 */
int lyra_columns(lyra_result_t result);

/**
 * Get column name
 * 
 * @param result Result handle
 * @param col Column index (0-based)
 * @return Column name, or NULL if invalid
 */
const char* lyra_column_name(lyra_result_t result, int col);

/**
 * Get column data type
 * 
 * @param result Result handle
 * @param col Column index (0-based)
 * @return Data type
 */
lyra_datatype_t lyra_column_type(lyra_result_t result, int col);

/**
 * Get integer value from result
 * 
 * @param result Result handle
 * @param row Row index (0-based)
 * @param col Column index (0-based)
 * @return Integer value
 */
int64_t lyra_get_int(lyra_result_t result, int64_t row, int col);

/**
 * Get double value from result
 * 
 * @param result Result handle
 * @param row Row index (0-based)
 * @param col Column index (0-based)
 * @return Double value
 */
double lyra_get_double(lyra_result_t result, int64_t row, int col);

/**
 * Get string value from result
 * 
 * @param result Result handle
 * @param row Row index (0-based)
 * @param col Column index (0-based)
 * @return String value (read-only)
 */
const char* lyra_get_string(lyra_result_t result, int64_t row, int col);

/**
 * Check if value is NULL
 * 
 * @param result Result handle
 * @param row Row index (0-based)
 * @param col Column index (0-based)
 * @return 1 if NULL, 0 otherwise
 */
int lyra_is_null(lyra_result_t result, int64_t row, int col);

/**
 * Get result as formatted JSON string
 * 
 * @param result Result handle
 * @return JSON string (read-only)
 */
const char* lyra_result_json(lyra_result_t result);

/**
 * Get result as CSV string
 * 
 * @param result Result handle
 * @return CSV string (read-only)
 */
const char* lyra_result_csv(lyra_result_t result);

/**
 * Free result resources
 * 
 * @param result Result handle
 * @return LYRA_OK on success
 */
lyra_errcode_t lyra_free_result(lyra_result_t result);

/**
 * TABLE MANAGEMENT
 */

/**
 * Create table with columns
 * 
 * @param db Database handle
 * @param table Table name
 * @param col_names Array of column names
 * @param col_types Array of column types
 * @param num_cols Number of columns
 * @param errmsg Pointer to error message
 * @return LYRA_OK on success
 */
lyra_errcode_t lyra_create_table(
    lyra_db_t db,
    const char* table,
    const char** col_names,
    const lyra_datatype_t* col_types,
    int num_cols,
    char** errmsg
);

/**
 * Drop table
 * 
 * @param db Database handle
 * @param table Table name
 * @param errmsg Pointer to error message
 * @return LYRA_OK on success
 */
lyra_errcode_t lyra_drop_table(lyra_db_t db, const char* table, char** errmsg);

/**
 * Get list of tables
 * 
 * @param db Database handle
 * @return Result handle with table names
 */
lyra_result_t lyra_list_tables(lyra_db_t db);

/**
 * Insert row into table
 * 
 * @param db Database handle
 * @param table Table name
 * @param col_names Column names
 * @param values Values (as strings)
 * @param num_cols Number of columns
 * @param errmsg Pointer to error message
 * @return LYRA_OK on success
 */
lyra_errcode_t lyra_insert(
    lyra_db_t db,
    const char* table,
    const char** col_names,
    const char** values,
    int num_cols,
    char** errmsg
);

/**
 * Insert row with typed values
 * 
 * @param db Database handle
 * @param table Table name
 * @param col_names Column names
 * @param col_types Column types (from lyra_datatype_t)
 * @param values Values (void* pointers to typed data)
 * @param num_cols Number of columns
 * @return LYRA_OK on success
 */
lyra_errcode_t lyra_insert_row(
    lyra_db_t db,
    const char* table,
    const char** col_names,
    const lyra_datatype_t* col_types,
    void** values,
    int num_cols
);

/**
 * INDEXING
 */

/**
 * Create index on column
 * 
 * @param db Database handle
 * @param table Table name
 * @param column Column name
 * @param index_type "btree", "hash", or "bitmap"
 * @param errmsg Pointer to error message
 * @return LYRA_OK on success
 */
lyra_errcode_t lyra_create_index(
    lyra_db_t db,
    const char* table,
    const char* column,
    const char* index_type,
    char** errmsg
);

/**
 * Drop index
 * 
 * @param db Database handle
 * @param index_name Index name
 * @param errmsg Pointer to error message
 * @return LYRA_OK on success
 */
lyra_errcode_t lyra_drop_index(lyra_db_t db, const char* index_name, char** errmsg);

/**
 * TRANSACTIONS
 */

/**
 * Begin transaction
 * 
 * @param db Database handle
 * @return LYRA_OK on success
 */
lyra_errcode_t lyra_begin(lyra_db_t db);

/**
 * Commit transaction
 * 
 * @param db Database handle
 * @return LYRA_OK on success
 */
lyra_errcode_t lyra_commit(lyra_db_t db);

/**
 * Rollback transaction
 * 
 * @param db Database handle
 * @return LYRA_OK on success
 */
lyra_errcode_t lyra_rollback(lyra_db_t db);

/**
 * PREPARED STATEMENTS
 */

/**
 * Prepare SQL statement
 * 
 * @param db Database handle
 * @param sql SQL statement with ? placeholders
 * @param errmsg Pointer to error message
 * @return Statement handle, or NULL on error
 */
lyra_stmt_t lyra_prepare(lyra_db_t db, const char* sql, char** errmsg);

/**
 * Bind integer parameter
 * 
 * @param stmt Statement handle
 * @param index Parameter index (1-based)
 * @param value Integer value
 * @return LYRA_OK on success
 */
lyra_errcode_t lyra_bind_int(lyra_stmt_t stmt, int index, int64_t value);

/**
 * Bind double parameter
 * 
 * @param stmt Statement handle
 * @param index Parameter index (1-based)
 * @param value Double value
 * @return LYRA_OK on success
 */
lyra_errcode_t lyra_bind_double(lyra_stmt_t stmt, int index, double value);

/**
 * Bind string parameter
 * 
 * @param stmt Statement handle
 * @param index Parameter index (1-based)
 * @param value String value
 * @return LYRA_OK on success
 */
lyra_errcode_t lyra_bind_string(lyra_stmt_t stmt, int index, const char* value);

/**
 * Execute prepared statement
 * 
 * @param stmt Statement handle
 * @param errmsg Pointer to error message
 * @return Result handle, or NULL on error
 */
lyra_result_t lyra_execute(lyra_stmt_t stmt, char** errmsg);

/**
 * Free statement resources
 * 
 * @param stmt Statement handle
 * @return LYRA_OK on success
 */
lyra_errcode_t lyra_free_stmt(lyra_stmt_t stmt);

/**
 * MEMORY & UTILITIES
 */

/**
 * Free memory allocated by LyraDB
 * 
 * @param ptr Pointer to free
 */
void lyra_free(void* ptr);

/**
 * Get library version
 * 
 * @return Version string (e.g., "0.85")
 */
const char* lyra_version(void);

/**
 * Get build information
 * 
 * @return Build info string
 */
const char* lyra_build_info(void);

#ifdef __cplusplus
}
#endif
