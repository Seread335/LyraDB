#include "lyradb_c.h"
#include "lyradb/database.h"
#include "lyradb/query_execution_engine.h"
#include <cstring>
#include <map>
#include <memory>
#include <sstream>

// Handle management
struct lyra_db_handle {
    std::shared_ptr<lyradb::Database> db;
    std::string last_error;
};

struct lyra_result_handle {
    std::vector<std::map<std::string, std::string>> rows;
    std::vector<std::string> columns;
    std::string json_cache;
    std::string csv_cache;
};

struct lyra_stmt_handle {
    std::shared_ptr<lyradb::Database> db;
    std::string sql;
    std::map<int, std::string> bindings;
};

// Global error handling
thread_local std::string g_last_error;

/* DATABASE MANAGEMENT */

lyra_db_t lyra_open(const char* path, char** errmsg) {
    try {
        auto handle = new lyra_db_handle();
        handle->db = std::make_shared<lyradb::Database>(path);
        return handle;
    } catch (const std::exception& e) {
        if (errmsg) {
            *errmsg = strdup(e.what());
        }
        return nullptr;
    }
}

lyra_errcode_t lyra_close(lyra_db_t db) {
    if (!db) return LYRA_ERROR;
    try {
        delete static_cast<lyra_db_handle*>(db);
        return LYRA_OK;
    } catch (...) {
        return LYRA_ERROR;
    }
}

const char* lyra_errmsg(lyra_db_t db) {
    if (!db) return "Invalid database handle";
    return static_cast<lyra_db_handle*>(db)->last_error.c_str();
}

int64_t lyra_filesize(lyra_db_t db) {
    if (!db) return -1;
    try {
        // TODO: Implement file size retrieval
        return 0;
    } catch (...) {
        return -1;
    }
}

lyra_errcode_t lyra_compact(lyra_db_t db) {
    if (!db) return LYRA_ERROR;
    try {
        // TODO: Implement compaction
        return LYRA_OK;
    } catch (...) {
        return LYRA_ERROR;
    }
}

/* QUERY EXECUTION */

lyra_result_t lyra_query(lyra_db_t db, const char* sql, char** errmsg) {
    if (!db || !sql) {
        if (errmsg) *errmsg = strdup("Invalid parameters");
        return nullptr;
    }

    try {
        auto handle = static_cast<lyra_db_handle*>(db);
        auto result = handle->db->query(sql);
        
        if (!result) {
            if (errmsg) *errmsg = strdup("Query execution failed");
            return nullptr;
        }

        auto res_handle = new lyra_result_handle();
        
        // Store column names
        res_handle->columns = result->column_names();
        
        // Convert result rows to string map format
        size_t rows = result->row_count();
        size_t cols = result->column_count();
        
        for (size_t r = 0; r < rows; r++) {
            std::map<std::string, std::string> row_map;
            
            for (size_t c = 0; c < cols && c < res_handle->columns.size(); c++) {
                const auto& col_name = res_handle->columns[c];
                
                // Get column data using the available API
                auto col_array = result->get_column(c);
                std::string value;
                if (col_array) {
                    // TODO: Extract value from Arrow array
                    value = "[value]";
                }
                
                row_map[col_name] = value;
            }
            
            res_handle->rows.push_back(row_map);
        }
        
        return res_handle;
    } catch (const std::exception& e) {
        if (errmsg) *errmsg = strdup(e.what());
        return nullptr;
    }
}

int64_t lyra_rows(lyra_result_t result) {
    if (!result) return 0;
    return static_cast<lyra_result_handle*>(result)->rows.size();
}

int lyra_columns(lyra_result_t result) {
    if (!result) return 0;
    return static_cast<lyra_result_handle*>(result)->columns.size();
}

const char* lyra_column_name(lyra_result_t result, int col) {
    if (!result) return nullptr;
    auto res = static_cast<lyra_result_handle*>(result);
    if (col < 0 || col >= (int)res->columns.size()) return nullptr;
    return res->columns[col].c_str();
}

lyra_datatype_t lyra_column_type(lyra_result_t result, int col) {
    if (!result) return LYRA_TYPE_NULL;
    // TODO: Store and return column types
    return LYRA_TYPE_STRING;
}

int64_t lyra_get_int(lyra_result_t result, int64_t row, int col) {
    if (!result) return 0;
    auto res = static_cast<lyra_result_handle*>(result);
    if (row < 0 || row >= (int64_t)res->rows.size()) return 0;
    if (col < 0 || col >= (int)res->columns.size()) return 0;
    
    const auto& col_name = res->columns[col];
    try {
        return std::stoll(res->rows[row][col_name]);
    } catch (...) {
        return 0;
    }
}

double lyra_get_double(lyra_result_t result, int64_t row, int col) {
    if (!result) return 0.0;
    auto res = static_cast<lyra_result_handle*>(result);
    if (row < 0 || row >= (int64_t)res->rows.size()) return 0.0;
    if (col < 0 || col >= (int)res->columns.size()) return 0.0;
    
    const auto& col_name = res->columns[col];
    try {
        return std::stod(res->rows[row][col_name]);
    } catch (...) {
        return 0.0;
    }
}

const char* lyra_get_string(lyra_result_t result, int64_t row, int col) {
    if (!result) return nullptr;
    auto res = static_cast<lyra_result_handle*>(result);
    if (row < 0 || row >= (int64_t)res->rows.size()) return nullptr;
    if (col < 0 || col >= (int)res->columns.size()) return nullptr;
    
    const auto& col_name = res->columns[col];
    return res->rows[row][col_name].c_str();
}

int lyra_is_null(lyra_result_t result, int64_t row, int col) {
    if (!result) return 1;
    auto res = static_cast<lyra_result_handle*>(result);
    if (row < 0 || row >= (int64_t)res->rows.size()) return 1;
    if (col < 0 || col >= (int)res->columns.size()) return 1;
    
    const auto& col_name = res->columns[col];
    return res->rows[row][col_name].empty() ? 1 : 0;
}

const char* lyra_result_json(lyra_result_t result) {
    if (!result) return "{}";
    auto res = static_cast<lyra_result_handle*>(result);
    // TODO: Generate JSON
    return res->json_cache.c_str();
}

const char* lyra_result_csv(lyra_result_t result) {
    if (!result) return "";
    auto res = static_cast<lyra_result_handle*>(result);
    // TODO: Generate CSV
    return res->csv_cache.c_str();
}

lyra_errcode_t lyra_free_result(lyra_result_t result) {
    if (!result) return LYRA_ERROR;
    try {
        delete static_cast<lyra_result_handle*>(result);
        return LYRA_OK;
    } catch (...) {
        return LYRA_ERROR;
    }
}

/* TABLE MANAGEMENT */

lyra_errcode_t lyra_create_table(
    lyra_db_t db,
    const char* table,
    const char** col_names,
    const lyra_datatype_t* col_types,
    int num_cols,
    char** errmsg) {
    if (!db || !table || !col_names || !col_types || num_cols <= 0) return LYRA_ERROR;
    
    try {
        auto handle = static_cast<lyra_db_handle*>(db);
        
        // Build CREATE TABLE statement
        std::stringstream ss;
        ss << "CREATE TABLE " << table << " (";
        
        for (int i = 0; i < num_cols; i++) {
            if (i > 0) ss << ", ";
            ss << col_names[i] << " ";
            
            // Convert datatype to SQL type string
            switch (col_types[i]) {
                case LYRA_TYPE_INT32:
                    ss << "INT";
                    break;
                case LYRA_TYPE_INT64:
                    ss << "BIGINT";
                    break;
                case LYRA_TYPE_FLOAT32:
                    ss << "FLOAT";
                    break;
                case LYRA_TYPE_FLOAT64:
                    ss << "DOUBLE";
                    break;
                case LYRA_TYPE_STRING:
                    ss << "VARCHAR(255)";
                    break;
                case LYRA_TYPE_BOOL:
                    ss << "BOOLEAN";
                    break;
                default:
                    ss << "TEXT";
            }
        }
        
        ss << ");";
        
        // Execute CREATE TABLE
        auto result = handle->db->query(ss.str());
        
        return result ? LYRA_OK : LYRA_ERROR;
    } catch (const std::exception& e) {
        if (errmsg) *errmsg = strdup(e.what());
        return LYRA_ERROR;
    }
}

lyra_errcode_t lyra_drop_table(lyra_db_t db, const char* table, char** errmsg) {
    if (!db || !table) return LYRA_ERROR;
    
    try {
        // TODO: Implement drop table
        return LYRA_OK;
    } catch (const std::exception& e) {
        if (errmsg) *errmsg = strdup(e.what());
        return LYRA_ERROR;
    }
}

lyra_result_t lyra_list_tables(lyra_db_t db) {
    if (!db) return nullptr;
    
    try {
        auto handle = static_cast<lyra_db_handle*>(db);
        auto tables = handle->db->list_tables();
        
        auto res_handle = new lyra_result_handle();
        res_handle->columns = {"table_name"};
        
        // Add each table as a row
        for (const auto& table_name : tables) {
            std::map<std::string, std::string> row;
            row["table_name"] = table_name;
            res_handle->rows.push_back(row);
        }
        
        return res_handle;
    } catch (...) {
        return nullptr;
    }
}

lyra_errcode_t lyra_insert(
    lyra_db_t db,
    const char* table,
    const char** col_names,
    const char** values,
    int num_cols,
    char** errmsg) {
    if (!db || !table || !col_names || !values) return LYRA_ERROR;
    
    try {
        auto handle = static_cast<lyra_db_handle*>(db);
        
        // Build INSERT statement
        std::stringstream ss;
        ss << "INSERT INTO " << table << " (";
        
        for (int i = 0; i < num_cols; i++) {
            if (i > 0) ss << ", ";
            ss << col_names[i];
        }
        
        ss << ") VALUES (";
        
        for (int i = 0; i < num_cols; i++) {
            if (i > 0) ss << ", ";
            ss << "'" << (values[i] ? values[i] : "") << "'";
        }
        
        ss << ");";
        
        std::string insert_sql = ss.str();
        // std::cerr << "DEBUG: INSERT SQL: " << insert_sql << std::endl;
        
        // Execute the INSERT statement
        auto result = handle->db->query(insert_sql);
        
        return result ? LYRA_OK : LYRA_ERROR;
    } catch (const std::exception& e) {
        if (errmsg) *errmsg = strdup(e.what());
        return LYRA_ERROR;
    }
}

lyra_errcode_t lyra_insert_row(
    lyra_db_t db,
    const char* table,
    const char** col_names,
    const lyra_datatype_t* col_types,
    void** values,
    int num_cols) {
    if (!db || !table || !col_names || !col_types || !values) return LYRA_ERROR;
    
    try {
        auto handle = static_cast<lyra_db_handle*>(db);
        
        // Build INSERT statement by converting typed values to strings
        std::stringstream ss;
        ss << "INSERT INTO " << table << " (";
        
        for (int i = 0; i < num_cols; i++) {
            if (i > 0) ss << ", ";
            ss << col_names[i];
        }
        
        ss << ") VALUES (";
        
        for (int i = 0; i < num_cols; i++) {
            if (i > 0) ss << ", ";
            
            if (!values[i]) {
                ss << "NULL";
            } else {
                // Convert based on column type
                switch (col_types[i]) {
                    case LYRA_TYPE_INT32: {
                        int val = *static_cast<int*>(values[i]);
                        ss << val;
                        break;
                    }
                    case LYRA_TYPE_INT64: {
                        long long val = *static_cast<long long*>(values[i]);
                        ss << val;
                        break;
                    }
                    case LYRA_TYPE_FLOAT32: {
                        float val = *static_cast<float*>(values[i]);
                        ss << val;
                        break;
                    }
                    case LYRA_TYPE_FLOAT64: {
                        double val = *static_cast<double*>(values[i]);
                        ss << val;
                        break;
                    }
                    case LYRA_TYPE_STRING: {
                        const char* str = static_cast<const char*>(values[i]);
                        ss << "'" << str << "'";
                        break;
                    }
                    case LYRA_TYPE_BOOL: {
                        bool val = *static_cast<bool*>(values[i]);
                        ss << (val ? 1 : 0);
                        break;
                    }
                    default:
                        ss << "NULL";
                }
            }
        }
        
        ss << ");";
        
        // Execute the INSERT statement
        auto result = handle->db->query(ss.str());
        
        return result ? LYRA_OK : LYRA_ERROR;
    } catch (...) {
        return LYRA_ERROR;
    }
}

/* INDEXING */

lyra_errcode_t lyra_create_index(
    lyra_db_t db,
    const char* table,
    const char* column,
    const char* index_type,
    char** errmsg) {
    if (!db || !table || !column) return LYRA_ERROR;
    
    try {
        // TODO: Implement index creation
        return LYRA_OK;
    } catch (const std::exception& e) {
        if (errmsg) *errmsg = strdup(e.what());
        return LYRA_ERROR;
    }
}

lyra_errcode_t lyra_drop_index(lyra_db_t db, const char* index_name, char** errmsg) {
    if (!db || !index_name) return LYRA_ERROR;
    
    try {
        // TODO: Implement drop index
        return LYRA_OK;
    } catch (const std::exception& e) {
        if (errmsg) *errmsg = strdup(e.what());
        return LYRA_ERROR;
    }
}

/* TRANSACTIONS */

lyra_errcode_t lyra_begin(lyra_db_t db) {
    if (!db) return LYRA_ERROR;
    try {
        // TODO: Implement begin
        return LYRA_OK;
    } catch (...) {
        return LYRA_ERROR;
    }
}

lyra_errcode_t lyra_commit(lyra_db_t db) {
    if (!db) return LYRA_ERROR;
    try {
        // TODO: Implement commit
        return LYRA_OK;
    } catch (...) {
        return LYRA_ERROR;
    }
}

lyra_errcode_t lyra_rollback(lyra_db_t db) {
    if (!db) return LYRA_ERROR;
    try {
        // TODO: Implement rollback
        return LYRA_OK;
    } catch (...) {
        return LYRA_ERROR;
    }
}

/* PREPARED STATEMENTS */

lyra_stmt_t lyra_prepare(lyra_db_t db, const char* sql, char** errmsg) {
    if (!db || !sql) {
        if (errmsg) *errmsg = strdup("Invalid parameters");
        return nullptr;
    }
    
    try {
        auto handle = static_cast<lyra_db_handle*>(db);
        auto stmt_handle = new lyra_stmt_handle();
        stmt_handle->db = handle->db;
        stmt_handle->sql = sql;
        return stmt_handle;
    } catch (const std::exception& e) {
        if (errmsg) *errmsg = strdup(e.what());
        return nullptr;
    }
}

lyra_errcode_t lyra_bind_int(lyra_stmt_t stmt, int index, int64_t value) {
    if (!stmt) return LYRA_ERROR;
    try {
        auto s = static_cast<lyra_stmt_handle*>(stmt);
        s->bindings[index] = std::to_string(value);
        return LYRA_OK;
    } catch (...) {
        return LYRA_ERROR;
    }
}

lyra_errcode_t lyra_bind_double(lyra_stmt_t stmt, int index, double value) {
    if (!stmt) return LYRA_ERROR;
    try {
        auto s = static_cast<lyra_stmt_handle*>(stmt);
        s->bindings[index] = std::to_string(value);
        return LYRA_OK;
    } catch (...) {
        return LYRA_ERROR;
    }
}

lyra_errcode_t lyra_bind_string(lyra_stmt_t stmt, int index, const char* value) {
    if (!stmt || !value) return LYRA_ERROR;
    try {
        auto s = static_cast<lyra_stmt_handle*>(stmt);
        s->bindings[index] = value;
        return LYRA_OK;
    } catch (...) {
        return LYRA_ERROR;
    }
}

lyra_result_t lyra_execute(lyra_stmt_t stmt, char** errmsg) {
    if (!stmt) {
        if (errmsg) *errmsg = strdup("Invalid statement");
        return nullptr;
    }
    
    try {
        auto s = static_cast<lyra_stmt_handle*>(stmt);
        // TODO: Replace bindings and execute
        auto result = s->db->query(s->sql);
        
        auto res_handle = new lyra_result_handle();
        // TODO: Convert result
        
        return res_handle;
    } catch (const std::exception& e) {
        if (errmsg) *errmsg = strdup(e.what());
        return nullptr;
    }
}

lyra_errcode_t lyra_free_stmt(lyra_stmt_t stmt) {
    if (!stmt) return LYRA_ERROR;
    try {
        delete static_cast<lyra_stmt_handle*>(stmt);
        return LYRA_OK;
    } catch (...) {
        return LYRA_ERROR;
    }
}

/* MEMORY & UTILITIES */

void lyra_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

const char* lyra_version(void) {
    return "0.85";
}

const char* lyra_build_info(void) {
    return "LyraDB Embedded Library v0.85 (C API) - Built Dec 2025";
}
