#include "lyradb/database.h"
#include "lyradb/table.h"
#include "lyradb/query_execution_engine.h"
#include "lyradb/sql_parser.h"
#include "lyradb/expression_evaluator.h"
#include "lyradb/hash_index_impl.h"
#include "lyradb/b_tree_impl.h"
#include <stdexcept>
#include <memory>
#include <map>
#include <unordered_map>
#include <algorithm>

namespace lyradb {

// ============================================================================
// Filter Pushdown Optimization - Phase 3.3.1
// ============================================================================

/**
 * @brief Check if an expression references only columns from a specific table
 * 
 * Returns true if the expression can be safely evaluated on just the primary table
 * without needing data from joined tables.
 */
static bool is_pushdown_compatible(
    const query::Expression* expr,
    const Schema& primary_table_schema) {
    
    if (!expr) return false;
    
    // Check if this is a binary expression
    auto binary = dynamic_cast<const query::BinaryExpr*>(expr);
    if (binary) {
        // AND expressions: both sides must be pushdown-compatible
        if (binary->op == query::BinaryOp::AND) {
            return is_pushdown_compatible(binary->left.get(), primary_table_schema) &&
                   is_pushdown_compatible(binary->right.get(), primary_table_schema);
        }
        
        // Comparison operators: check if all columns are in primary table
        if (binary->op == query::BinaryOp::EQUAL || 
            binary->op == query::BinaryOp::NOT_EQUAL || 
            binary->op == query::BinaryOp::LESS || 
            binary->op == query::BinaryOp::GREATER || 
            binary->op == query::BinaryOp::LESS_EQUAL || 
            binary->op == query::BinaryOp::GREATER_EQUAL ||
            binary->op == query::BinaryOp::LIKE || 
            binary->op == query::BinaryOp::IN) {
            
            // Check left side
            auto left_col = dynamic_cast<const query::ColumnRefExpr*>(binary->left.get());
            if (left_col) {
                for (size_t i = 0; i < primary_table_schema.num_columns(); ++i) {
                    if (primary_table_schema.get_column(i).name == left_col->column_name) {
                        return true;  // Simple column comparison - pushdown-able
                    }
                }
            }
        }
    }
    
    return false;
}

// ============================================================================
// Hash Join Implementation - Phase 3.3.2
// ============================================================================

/**
 * @brief Extract join key columns from a join condition
 * 
 * For equality conditions like table1.col = table2.col, extracts:
 * - left_key: the column from the left table
 * - right_key: the column from the right table
 * 
 * Returns true if extraction was successful, false otherwise
 */
static bool extract_join_keys_from_condition(
    const query::Expression* condition,
    std::vector<std::string>& left_keys,
    std::vector<std::string>& right_keys) {
    
    if (!condition) return false;
    
    auto binary = dynamic_cast<const query::BinaryExpr*>(condition);
    if (!binary) return false;
    
    // Handle AND expressions - extract keys from all equality conditions
    if (binary->op == query::BinaryOp::AND) {
        bool left_extracted = extract_join_keys_from_condition(
            binary->left.get(), left_keys, right_keys);
        bool right_extracted = extract_join_keys_from_condition(
            binary->right.get(), left_keys, right_keys);
        return left_extracted || right_extracted;
    }
    
    // Handle equality conditions: column = column
    if (binary->op == query::BinaryOp::EQUAL) {
        auto left_col = dynamic_cast<const query::ColumnRefExpr*>(binary->left.get());
        auto right_col = dynamic_cast<const query::ColumnRefExpr*>(binary->right.get());
        
        if (left_col && right_col) {
            // Both sides are column references - extract them
            left_keys.push_back(left_col->column_name);
            right_keys.push_back(right_col->column_name);
            return true;
        }
    }
    
    return false;
}

// Helper: Extract join key value from row data
static std::string extract_join_key(const std::vector<std::string>& row, 
                                     const Schema& schema, 
                                     const std::string& key_column) {
    for (size_t i = 0; i < schema.num_columns() && i < row.size(); ++i) {
        if (schema.get_column(i).name == key_column) {
            return row[i];
        }
    }
    return "";
}

// Helper: Extract all join keys from a row
static std::string extract_join_keys(const std::vector<std::string>& row,
                                      const Schema& schema,
                                      const std::vector<std::string>& join_keys) {
    std::string combined_key;
    for (const auto& key : join_keys) {
        for (size_t i = 0; i < schema.num_columns() && i < row.size(); ++i) {
            if (schema.get_column(i).name == key) {
                if (!combined_key.empty()) combined_key += "|";
                combined_key += row[i];
                break;
            }
        }
    }
    return combined_key;
}

// Perform hash join between two sets of rows
static std::vector<std::vector<std::string>> hash_join(
    const std::vector<std::vector<std::string>>& left_rows,
    const Schema& left_schema,
    const std::vector<std::vector<std::string>>& right_rows,
    const Schema& right_schema,
    const std::vector<std::string>& left_join_keys,
    const std::vector<std::string>& right_join_keys,
    bool is_left_join = false) {
    
    std::vector<std::vector<std::string>> result;
    
    // Build hash table on smaller table (right table)
    std::unordered_map<std::string, std::vector<std::vector<std::string>>> hash_table;
    
    for (const auto& right_row : right_rows) {
        std::string key = extract_join_keys(right_row, right_schema, right_join_keys);
        hash_table[key].push_back(right_row);
    }
    
    // Probe hash table with left rows
    for (const auto& left_row : left_rows) {
        std::string left_key = extract_join_keys(left_row, left_schema, left_join_keys);
        
        auto it = hash_table.find(left_key);
        if (it != hash_table.end()) {
            // Match found - add all matching right rows
            for (const auto& right_row : it->second) {
                auto joined_row = left_row;
                joined_row.insert(joined_row.end(), right_row.begin(), right_row.end());
                result.push_back(joined_row);
            }
        } else if (is_left_join) {
            // No match for LEFT JOIN - add with NULL padding
            auto joined_row = left_row;
            for (size_t i = 0; i < right_schema.num_columns(); ++i) {
                joined_row.push_back("");  // NULL representation
            }
            result.push_back(joined_row);
        }
    }
    
    return result;
}

Database::Database(const std::string& path) : path_(path) {
    is_open_ = true;
    engine_ = std::make_unique<QueryExecutionEngine>(this);
}

Database::~Database() {
    if (is_open_) {
        close();
    }
}

void Database::create_table(const std::string& name, const Schema& schema) {
    if (tables_.find(name) != tables_.end()) {
        throw std::runtime_error("Table already exists: " + name);
    }
    
    tables_[name] = std::make_shared<Table>(name, schema);
}

std::shared_ptr<Table> Database::get_table(const std::string& name) {
    auto it = tables_.find(name);
    if (it == tables_.end()) {
        throw std::runtime_error("Table not found: " + name);
    }
    return it->second;
}

std::unique_ptr<QueryResult> Database::query(const std::string& sql) {
    // ========================================================================
    // PHASE 3.4: QUERY RESULT CACHING
    // Check cache for SELECT queries before execution
    // ========================================================================
    
    // Parse first to determine query type
    query::SqlParser parser;
    auto statement = parser.parse(sql);
    
    if (!statement) {
        throw std::runtime_error("Failed to parse SQL: " + parser.get_last_error());
    }
    
    // Check if this is a SELECT query (can be cached)
    auto select_stmt = dynamic_cast<query::SelectStatement*>(statement.get());
    if (select_stmt && query_cache_.is_enabled()) {
        // Try to get from cache
        if (auto cached_result = query_cache_.get(sql)) {
            // Cache hit - return cloned result as unique_ptr
            auto engine_result = std::dynamic_pointer_cast<EngineQueryResult>(cached_result);
            if (engine_result) {
                return std::make_unique<EngineQueryResult>(*engine_result);
            }
        }
    }
    
    // Not cached or not SELECT - execute directly
    auto result = execute(sql);
    
    // Cache SELECT results
    if (select_stmt && query_cache_.is_enabled()) {
        std::set<std::string> affected_tables;
        if (select_stmt->from_table) {
            affected_tables.insert(select_stmt->from_table->table_name);
        }
        for (const auto& join : select_stmt->joins) {
            affected_tables.insert(join.table.table_name);
        }
        // Convert unique_ptr to shared_ptr for caching
        auto shared_result = std::make_shared<EngineQueryResult>(*dynamic_cast<EngineQueryResult*>(result.get()));
        query_cache_.put(sql, shared_result, affected_tables);
    }
    
    return result;
}

std::unique_ptr<QueryResult> Database::execute(const std::string& sql) {
    // Actual query execution (was in query() method before caching)
    // Parse the SQL statement
    query::SqlParser parser;
    auto statement = parser.parse(sql);
    
    if (!statement) {
        throw std::runtime_error("Failed to parse SQL: " + parser.get_last_error());
    }
    
    // Handle CREATE TABLE
    auto create_stmt = dynamic_cast<query::CreateTableStatement*>(statement.get());
    if (create_stmt) {
        // Build schema from parsed columns
        std::vector<ColumnDef> col_defs;
        
        for (const auto& col : create_stmt->columns) {
            DataType data_type;
            
            // Convert string type name to DataType
            if (col.data_type == "INT") {
                data_type = DataType::INT32;
            } else if (col.data_type == "BIGINT") {
                data_type = DataType::INT64;
            } else if (col.data_type == "FLOAT") {
                data_type = DataType::FLOAT32;
            } else if (col.data_type == "DOUBLE") {
                data_type = DataType::FLOAT64;
            } else if (col.data_type == "VARCHAR") {
                data_type = DataType::STRING;
            } else if (col.data_type == "BOOL") {
                data_type = DataType::BOOL;
            } else {
                throw std::runtime_error("Unknown data type: " + col.data_type);
            }
            
            col_defs.emplace_back(col.column_name, data_type);
        }
        
        // Create the table
        Schema schema(col_defs);
        create_table(create_stmt->table_name, schema);
        
        return nullptr;  // CREATE TABLE returns null result
    }
    
    // Handle INSERT
    auto insert_stmt = dynamic_cast<query::InsertStatement*>(statement.get());
    if (insert_stmt) {
        auto table = get_table(insert_stmt->table_name);
        
        // Invalidate cache for this table (mutation detected)
        query_cache_.invalidate(insert_stmt->table_name);
        
        // For each row of values
        for (const auto& row_values : insert_stmt->values) {
            // Convert expression values to void* for table insertion
            std::vector<void*> typed_values;
            std::vector<std::string> string_values;
            
            // Get table schema to know data types
            const Schema& schema = table->get_schema();
            
            for (size_t i = 0; i < row_values.size() && i < schema.num_columns(); ++i) {
                // Get the expression value
                auto literal_expr = dynamic_cast<query::LiteralExpr*>(row_values[i].get());
                if (literal_expr) {
                    string_values.push_back(literal_expr->value.value);
                } else {
                    string_values.push_back("");
                }
            }
            
            // Get row ID before insertion (will be current row count)
            size_t new_row_id = table->row_count();
            
            // Insert row as strings
            table->insert_row(string_values);
            
            // Update all indexes (single-column and composite)
            index::update_table_indexes(
                insert_stmt->table_name,
                new_row_id,
                string_values,
                schema);
            index::update_composite_table_indexes(
                insert_stmt->table_name,
                new_row_id,
                string_values,
                schema);
        }
        
        return nullptr;  // INSERT returns null result
    }
    
    // Handle UPDATE
    // Updates rows in a table based on column assignments and optional WHERE clause
    // Returns a QueryResult with affected_rows count
    auto update_stmt = dynamic_cast<query::UpdateStatement*>(statement.get());
    if (update_stmt) {
        auto table = get_table(update_stmt->table_name);
        
        // Invalidate cache for this table (mutation detected)
        query_cache_.invalidate(update_stmt->table_name);
        
        const Schema& schema = table->get_schema();
        
        // Create expression evaluator for WHERE clause and assignment expressions
        ExpressionEvaluator evaluator;
        
        int rows_affected = 0;
        std::vector<std::vector<std::string>> all_rows = table->scan_all();
        
        // Process each row
        for (size_t i = 0; i < all_rows.size(); ++i) {
            const auto& row = all_rows[i];
            
            // Build row data map (column_name -> value) for expression evaluation
            // This allows WHERE clause and RHS expressions to reference column values
            RowData row_data;
            for (size_t col_idx = 0; col_idx < schema.num_columns(); ++col_idx) {
                row_data[schema.get_column(col_idx).name] = row[col_idx];
            }
            
            // Evaluate WHERE clause for this row
            // If WHERE is present, evaluate condition; otherwise update all rows
            bool should_update = true;
            if (update_stmt->where_clause) {
                evaluator.set_context_row(row_data);
                auto where_result = evaluator.evaluate(update_stmt->where_clause.get(), row_data);
                
                // Convert result to boolean
                should_update = false;
                if (std::holds_alternative<bool>(where_result)) {
                    should_update = std::get<bool>(where_result);
                } else if (std::holds_alternative<int64_t>(where_result)) {
                    should_update = std::get<int64_t>(where_result) != 0;
                } else if (std::holds_alternative<double>(where_result)) {
                    should_update = std::get<double>(where_result) != 0.0;
                } else if (std::holds_alternative<std::string>(where_result)) {
                    should_update = !std::get<std::string>(where_result).empty();
                }
            }
            
            if (should_update) {
                // Update the row with new values
                std::vector<std::string> updated_row = row;
                
                // Apply each assignment
                for (const auto& [col_name, expr] : update_stmt->assignments) {
                    size_t col_idx = schema.column_index(col_name);
                    
                    // Evaluate assignment expression
                    evaluator.set_context_row(row_data);
                    auto new_value = evaluator.evaluate(expr.get(), row_data);
                    
                    // Convert result to string
                    std::string str_value;
                    if (std::holds_alternative<int64_t>(new_value)) {
                        str_value = std::to_string(std::get<int64_t>(new_value));
                    } else if (std::holds_alternative<double>(new_value)) {
                        str_value = std::to_string(std::get<double>(new_value));
                    } else if (std::holds_alternative<bool>(new_value)) {
                        str_value = std::get<bool>(new_value) ? "true" : "false";
                    } else if (std::holds_alternative<std::string>(new_value)) {
                        str_value = std::get<std::string>(new_value);
                    } else {
                        str_value = "";
                    }
                    
                    updated_row[col_idx] = str_value;
                    row_data[col_name] = str_value;
                }
                
                // Update the row in the table
                table->update_row(i, updated_row);
                rows_affected++;
            }
        }
        
        // Return result with affected row count
        auto result = std::make_unique<EngineQueryResult>();
        result->set_affected_rows(rows_affected);
        return result;
    }
    
    // Handle DELETE
    // Deletes rows from a table based on optional WHERE clause
    // Returns a QueryResult with affected_rows count
    auto delete_stmt = dynamic_cast<query::DeleteStatement*>(statement.get());
    if (delete_stmt) {
        auto table = get_table(delete_stmt->table_name);
        
        // Invalidate cache for this table (mutation detected)
        query_cache_.invalidate(delete_stmt->table_name);
        
        const Schema& schema = table->get_schema();
        
        // Create expression evaluator for WHERE clause
        ExpressionEvaluator evaluator;
        
        std::vector<size_t> rows_to_delete;
        std::vector<std::vector<std::string>> all_rows = table->scan_all();
        
        // Find rows to delete by evaluating WHERE clause for each row
        for (size_t i = 0; i < all_rows.size(); ++i) {
            const auto& row = all_rows[i];
            
            // Build row data map (column_name -> value) for expression evaluation
            RowData row_data;
            for (size_t col_idx = 0; col_idx < schema.num_columns(); ++col_idx) {
                row_data[schema.get_column(col_idx).name] = row[col_idx];
            }
            
            // Evaluate WHERE clause for this row
            // If WHERE is present, evaluate condition; otherwise delete all rows
            bool should_delete = true;
            if (delete_stmt->where_clause) {
                evaluator.set_context_row(row_data);
                auto where_result = evaluator.evaluate(delete_stmt->where_clause.get(), row_data);
                
                // Convert result to boolean
                should_delete = false;
                if (std::holds_alternative<bool>(where_result)) {
                    should_delete = std::get<bool>(where_result);
                } else if (std::holds_alternative<int64_t>(where_result)) {
                    should_delete = std::get<int64_t>(where_result) != 0;
                } else if (std::holds_alternative<double>(where_result)) {
                    should_delete = std::get<double>(where_result) != 0.0;
                } else if (std::holds_alternative<std::string>(where_result)) {
                    should_delete = !std::get<std::string>(where_result).empty();
                }
            }
            
            if (should_delete) {
                rows_to_delete.push_back(i);
            }
        }
        
        // Delete the rows
        int rows_affected = rows_to_delete.size();
        table->delete_rows(rows_to_delete);
        
        // Update all indexes (single-column and composite)
        index::remove_from_table_indexes(delete_stmt->table_name, rows_to_delete);
        index::remove_from_composite_table_indexes(delete_stmt->table_name, rows_to_delete);
        
        // Return result with affected row count
        auto result = std::make_unique<EngineQueryResult>();
        result->set_affected_rows(rows_affected);
        return result;
    }
    
    // Handle CREATE INDEX
    auto create_index_stmt = dynamic_cast<query::CreateIndexStatement*>(statement.get());
    if (create_index_stmt) {
        auto table = get_table(create_index_stmt->table_name);
        const Schema& schema = table->get_schema();
        
        if (!create_index_stmt->columns.empty()) {
            auto rows = table->scan_all();
            
            // Check if this is a single-column or multi-column index
            if (create_index_stmt->columns.size() == 1) {
                // Single-column index
                const auto& column_name = create_index_stmt->columns[0];
                
                // Determine index type: B-TREE for range queries, HASH (default) for exact match
                // For now, we'll use HASH by default and add B-TREE support via index type specification
                // Default to hash index for backward compatibility
                index_manager_.create_hash_index(
                    create_index_stmt->index_name,
                    create_index_stmt->table_name,
                    column_name);
                
                // Build the hash index from table data
                index::build_hash_index(
                    create_index_stmt->index_name,
                    create_index_stmt->table_name,
                    column_name,
                    rows,
                    schema);
                    
                // Optionally build B-tree as secondary index for range queries
                // This allows both exact-match (via hash) and range (via B-tree) optimization
                try {
                    index::build_btree_index(
                        create_index_stmt->index_name + "_btree",
                        create_index_stmt->table_name,
                        column_name,
                        rows,
                        schema);
                } catch (...) {
                    // B-tree index is optional; silently fail if type mismatch
                }
            } else {
                // Multi-column index (Phase 4.1.2)
                // Create composite hash index
                index_manager_.create_hash_index(
                    create_index_stmt->index_name,
                    create_index_stmt->table_name,
                    create_index_stmt->columns[0]);  // For manager tracking, use first column
                
                // Build the composite hash index from table data
                index::build_composite_hash_index(
                    create_index_stmt->index_name,
                    create_index_stmt->table_name,
                    create_index_stmt->columns,
                    rows,
                    schema);
                    
                // Optionally build composite B-tree as secondary index for range queries
                try {
                    index::build_composite_btree_index(
                        create_index_stmt->index_name + "_btree",
                        create_index_stmt->table_name,
                        create_index_stmt->columns,
                        rows,
                        schema);
                } catch (...) {
                    // B-tree index is optional; silently fail if type mismatch
                }
            }
        }
        
        return nullptr;  // CREATE INDEX returns null result
    }
    
    // Handle DROP
    auto drop_stmt = dynamic_cast<query::DropStatement*>(statement.get());
    if (drop_stmt) {
        if (drop_stmt->type == query::DropStatement::TABLE) {
            // Drop table if exists
            auto it = tables_.find(drop_stmt->object_name);
            if (it != tables_.end()) {
                tables_.erase(it);
                
                // Clear all indexes on this table
                index::clear_table_indexes(drop_stmt->object_name);
                index::clear_composite_table_indexes(drop_stmt->object_name);
            } else if (!drop_stmt->if_exists) {
                throw std::runtime_error("Table not found: " + drop_stmt->object_name);
            }
        } else {
            // Drop index (TODO: implement index manager drop)
        }
        
        return nullptr;  // DROP returns null result
    }
    
    // Handle SELECT
    auto select_stmt = dynamic_cast<query::SelectStatement*>(statement.get());
    if (select_stmt) {
        // Get all tables and scan for SELECT results
        // For now, return a simple in-memory result with table data
        
        if (select_stmt->from_table && !select_stmt->from_table->table_name.empty()) {
            auto table = get_table(select_stmt->from_table->table_name);
            const Schema& schema = table->get_schema();
            
            // Get initial column names and schemas for tracking all tables in join
            std::vector<std::string> col_names;
            std::map<std::string, const Schema*> table_schemas;
            
            for (size_t i = 0; i < schema.num_columns(); ++i) {
                col_names.push_back(schema.get_column(i).name);
            }
            table_schemas[select_stmt->from_table->table_name] = &schema;
            
            // Get rows
            auto rows = table->scan_all();
            
            // ========================================================================
            // FILTER PUSHDOWN OPTIMIZATION (Phase 3.3.1)
            // Apply table-specific WHERE filters BEFORE JOIN to reduce join size
            // This is critical for performance: if WHERE filters 90% of rows,
            // we only need to join 10% instead of 100%
            // ========================================================================
            if (select_stmt->where_clause && !select_stmt->joins.empty()) {
                // Check if the WHERE clause can be pushed down to the primary table
                if (is_pushdown_compatible(select_stmt->where_clause.get(), schema)) {
                    // Apply filter early - BEFORE JOIN
                    ExpressionEvaluator evaluator;
                    std::vector<std::vector<std::string>> filtered_rows;
                    
                    for (const auto& row : rows) {
                        RowData row_data;
                        for (size_t i = 0; i < schema.num_columns() && i < row.size(); ++i) {
                            row_data[schema.get_column(i).name] = row[i];
                        }
                        
                        auto result = evaluator.evaluate(select_stmt->where_clause.get(), row_data);
                        bool condition_met = false;
                        
                        if (std::holds_alternative<bool>(result)) {
                            condition_met = std::get<bool>(result);
                        } else if (std::holds_alternative<int64_t>(result)) {
                            condition_met = std::get<int64_t>(result) != 0;
                        } else if (std::holds_alternative<double>(result)) {
                            condition_met = std::get<double>(result) != 0.0;
                        }
                        
                        if (condition_met) {
                            filtered_rows.push_back(row);
                        }
                    }
                    
                    // Update rows with filtered results
                    rows = filtered_rows;
                    // Mark that WHERE clause was applied so we don't apply it again after JOIN
                    select_stmt->where_clause.reset();
                }
            } else if (select_stmt->where_clause && select_stmt->joins.empty()) {
                // No joins - apply WHERE clause now
                ExpressionEvaluator evaluator;
                std::vector<std::vector<std::string>> filtered_rows;
                
                for (const auto& row : rows) {
                    RowData row_data;
                    for (size_t i = 0; i < schema.num_columns() && i < row.size(); ++i) {
                        row_data[schema.get_column(i).name] = row[i];
                    }
                    
                    auto result = evaluator.evaluate(select_stmt->where_clause.get(), row_data);
                    bool condition_met = false;
                    
                    if (std::holds_alternative<bool>(result)) {
                        condition_met = std::get<bool>(result);
                    } else if (std::holds_alternative<int64_t>(result)) {
                        condition_met = std::get<int64_t>(result) != 0;
                    } else if (std::holds_alternative<double>(result)) {
                        condition_met = std::get<double>(result) != 0.0;
                    }
                    
                    if (condition_met) {
                        filtered_rows.push_back(row);
                    }
                }
                
                rows = filtered_rows;
                select_stmt->where_clause.reset();
            }
            
            // Handle JOINs if present (using HASH JOIN for better performance)
            if (!select_stmt->joins.empty()) {
                ExpressionEvaluator evaluator;
                
                for (const auto& join : select_stmt->joins) {
                    auto join_table = get_table(join.table.table_name);
                    const Schema& join_schema = join_table->get_schema();
                    
                    auto join_rows = join_table->scan_all();
                    std::vector<std::vector<std::string>> joined_rows;
                    bool is_left_join = (join.join_type == query::JoinType::LEFT);
                    
                    // ====================================================================
                    // HASH JOIN OPTIMIZATION (Phase 3.3.2)
                    // Try to extract join keys for efficient O(n+m) hash join
                    // Falls back to nested loop join for complex conditions
                    // ====================================================================
                    std::vector<std::string> left_join_keys;
                    std::vector<std::string> right_join_keys;
                    bool keys_extracted = extract_join_keys_from_condition(
                        join.join_condition.get(),
                        left_join_keys,
                        right_join_keys);
                    
                    if (keys_extracted && !left_join_keys.empty() && !right_join_keys.empty()) {
                        // Use hash join for equality conditions
                        // Build hash table on right table (smaller is better)
                        std::unordered_map<std::string, std::vector<std::vector<std::string>>> hash_table;
                        
                        for (const auto& right_row : join_rows) {
                            std::string hash_key = extract_join_keys(right_row, join_schema, right_join_keys);
                            hash_table[hash_key].push_back(right_row);
                        }
                        
                        // Probe hash table with left rows
                        for (const auto& left_row : rows) {
                            std::string left_key = extract_join_keys(left_row, schema, left_join_keys);
                            
                            auto it = hash_table.find(left_key);
                            if (it != hash_table.end()) {
                                // Match found - add all matching right rows
                                for (const auto& right_row : it->second) {
                                    auto merged_row = left_row;
                                    merged_row.insert(merged_row.end(), right_row.begin(), right_row.end());
                                    joined_rows.push_back(merged_row);
                                }
                            } else if (is_left_join) {
                                // No match for LEFT JOIN - add with NULL padding
                                auto merged_row = left_row;
                                for (size_t i = 0; i < join_schema.num_columns(); ++i) {
                                    merged_row.push_back("");  // NULL representation
                                }
                                joined_rows.push_back(merged_row);
                            }
                        }
                    } else {
                        // Fall back to nested loop join for complex conditions
                        ExpressionEvaluator evaluator;
                        
                        for (const auto& left_row : rows) {
                            RowData left_data;
                            for (size_t i = 0; i < schema.num_columns() && i < left_row.size(); ++i) {
                                left_data[schema.get_column(i).name] = left_row[i];
                            }
                            
                            bool match_found = false;
                            
                            for (const auto& right_row : join_rows) {
                                RowData right_data;
                                for (size_t i = 0; i < join_schema.num_columns() && i < right_row.size(); ++i) {
                                    right_data[join_schema.get_column(i).name] = right_row[i];
                                }
                                
                                // Merge contexts for join condition evaluation
                                RowData merged_data = left_data;
                                for (const auto& [key, val] : right_data) {
                                    merged_data[key] = val;
                                }
                                
                                // Evaluate join condition
                                auto condition_result = evaluator.evaluate(join.join_condition.get(), merged_data);
                                bool condition_met = false;
                                
                                if (std::holds_alternative<bool>(condition_result)) {
                                    condition_met = std::get<bool>(condition_result);
                                } else if (std::holds_alternative<int64_t>(condition_result)) {
                                    condition_met = std::get<int64_t>(condition_result) != 0;
                                }
                                
                                if (condition_met) {
                                    // INNER/LEFT JOIN: include row
                                    auto merged_row = left_row;
                                    merged_row.insert(merged_row.end(), right_row.begin(), right_row.end());
                                    joined_rows.push_back(merged_row);
                                    match_found = true;
                                }
                            }
                            
                            // Handle LEFT JOIN with NULL padding
                            if (!match_found && is_left_join) {
                                auto merged_row = left_row;
                                for (size_t i = 0; i < join_schema.num_columns(); ++i) {
                                    merged_row.push_back("");  // NULL representation
                                }
                                joined_rows.push_back(merged_row);
                            }
                        }
                    }
                    
                    // Add joined table columns to col_names
                    for (size_t i = 0; i < join_schema.num_columns(); ++i) {
                        col_names.push_back(join_schema.get_column(i).name);
                    }
                    
                    rows = joined_rows;
                }
            }
            
            // Filter by WHERE clause if present
            if (select_stmt->where_clause) {
                ExpressionEvaluator evaluator;
                std::vector<std::vector<std::string>> filtered_rows;
                
                for (const auto& row : rows) {
                    // Create RowData from row
                    RowData row_data;
                    for (size_t i = 0; i < schema.num_columns() && i < row.size(); ++i) {
                        row_data[schema.get_column(i).name] = row[i];
                    }
                    
                    // Evaluate WHERE condition
                    auto result = evaluator.evaluate(select_stmt->where_clause.get(), row_data);
                    if (std::holds_alternative<bool>(result) && std::get<bool>(result)) {
                        filtered_rows.push_back(row);
                    }
                }
                rows = filtered_rows;
            }
            
            // Handle GROUP BY if present
            if (!select_stmt->group_by_list.empty()) {
                // Create map for grouping: grouping_key -> list of rows in group
                std::map<std::string, std::vector<std::vector<std::string>>> groups;
                ExpressionEvaluator evaluator;
                
                // Group rows
                for (const auto& row : rows) {
                    // Create RowData for this row
                    RowData row_data;
                    for (size_t i = 0; i < schema.num_columns() && i < row.size(); ++i) {
                        row_data[schema.get_column(i).name] = row[i];
                    }
                    
                    // Create grouping key from GROUP BY expressions
                    std::string key;
                    for (size_t i = 0; i < select_stmt->group_by_list.size(); ++i) {
                        if (i > 0) key += "|";
                        auto expr_value = evaluator.evaluate(select_stmt->group_by_list[i].get(), row_data);
                        
                        // Convert ExpressionValue to string
                        if (std::holds_alternative<nullptr_t>(expr_value)) {
                            key += "NULL";
                        } else if (std::holds_alternative<int64_t>(expr_value)) {
                            key += std::to_string(std::get<int64_t>(expr_value));
                        } else if (std::holds_alternative<double>(expr_value)) {
                            key += std::to_string(std::get<double>(expr_value));
                        } else if (std::holds_alternative<std::string>(expr_value)) {
                            key += std::get<std::string>(expr_value);
                        } else if (std::holds_alternative<bool>(expr_value)) {
                            key += std::get<bool>(expr_value) ? "true" : "false";
                        }
                    }
                    
                    groups[key].push_back(row);
                }
                
                // Build result rows from groups with aggregates
                std::vector<std::vector<std::string>> grouped_rows;
                
                for (const auto& [key, group_rows] : groups) {
                    // Start with the first row (for GROUP BY columns)
                    auto result_row = group_rows[0];
                    
                    // TODO: Process aggregates in select_list
                    // For now, just return the first row of each group
                    
                    // Apply HAVING clause if present
                    if (select_stmt->having_clause) {
                        // Create RowData from the group result for HAVING evaluation
                        RowData group_data;
                        for (size_t i = 0; i < schema.num_columns() && i < result_row.size(); ++i) {
                            group_data[schema.get_column(i).name] = result_row[i];
                        }
                        
                        // Evaluate HAVING condition
                        auto having_result = evaluator.evaluate(select_stmt->having_clause.get(), group_data);
                        
                        // Only include group if HAVING condition is true
                        if (!std::holds_alternative<bool>(having_result) || !std::get<bool>(having_result)) {
                            continue;  // Skip this group
                        }
                    }
                    
                    grouped_rows.push_back(result_row);
                }
                
                rows = grouped_rows;
            }
            
            // Handle ORDER BY if present
            if (!select_stmt->order_by_list.empty()) {
                ExpressionEvaluator evaluator;
                
                // ====================================================================
                // PARTIAL SORT OPTIMIZATION (Phase 3.3.3)
                // When LIMIT is specified, use partial_sort instead of full sort
                // Performance: O(n log k) instead of O(n log n) where k = LIMIT
                // ====================================================================
                int64_t limit = select_stmt->limit;
                bool use_partial_sort = (limit > 0 && limit < static_cast<int64_t>(rows.size()));
                
                if (use_partial_sort) {
                    // Partial sort: only sort first 'limit' rows
                    std::partial_sort(
                        rows.begin(),
                        rows.begin() + std::min(static_cast<size_t>(limit), rows.size()),
                        rows.end(),
                        [&](const std::vector<std::string>& row_a, const std::vector<std::string>& row_b) -> bool {
                            // Same comparison logic as full sort
                            for (const auto& sort_key : select_stmt->order_by_list) {
                                RowData row_a_data, row_b_data;
                                
                                for (size_t i = 0; i < schema.num_columns() && i < row_a.size(); ++i) {
                                    row_a_data[schema.get_column(i).name] = row_a[i];
                                }
                                for (size_t i = 0; i < schema.num_columns() && i < row_b.size(); ++i) {
                                    row_b_data[schema.get_column(i).name] = row_b[i];
                                }
                                
                                auto val_a = evaluator.evaluate(sort_key.expression.get(), row_a_data);
                                auto val_b = evaluator.evaluate(sort_key.expression.get(), row_b_data);
                                
                                bool less_than = false;
                                bool greater_than = false;
                                
                                if (std::holds_alternative<int64_t>(val_a) && std::holds_alternative<int64_t>(val_b)) {
                                    int64_t a = std::get<int64_t>(val_a);
                                    int64_t b = std::get<int64_t>(val_b);
                                    less_than = (a < b);
                                    greater_than = (a > b);
                                } else if (std::holds_alternative<double>(val_a) && std::holds_alternative<double>(val_b)) {
                                    double a = std::get<double>(val_a);
                                    double b = std::get<double>(val_b);
                                    less_than = (a < b);
                                    greater_than = (a > b);
                                } else if (std::holds_alternative<std::string>(val_a) && std::holds_alternative<std::string>(val_b)) {
                                    const auto& a = std::get<std::string>(val_a);
                                    const auto& b = std::get<std::string>(val_b);
                                    less_than = (a < b);
                                    greater_than = (a > b);
                                }
                                
                                if (less_than) {
                                    return (sort_key.direction == query::SortDirection::ASC);
                                } else if (greater_than) {
                                    return (sort_key.direction == query::SortDirection::DESC);
                                }
                            }
                            return false;
                        }
                    );
                } else {
                    // Full sort when no LIMIT or LIMIT > rows
                    std::sort(rows.begin(), rows.end(), 
                        [&](const std::vector<std::string>& row_a, const std::vector<std::string>& row_b) -> bool {
                            // Evaluate each sort key
                            for (const auto& sort_key : select_stmt->order_by_list) {
                                // Create RowData for both rows
                                RowData row_a_data, row_b_data;
                                
                                for (size_t i = 0; i < schema.num_columns() && i < row_a.size(); ++i) {
                                    row_a_data[schema.get_column(i).name] = row_a[i];
                                }
                                for (size_t i = 0; i < schema.num_columns() && i < row_b.size(); ++i) {
                                    row_b_data[schema.get_column(i).name] = row_b[i];
                                }
                                
                                // Evaluate sort expression for both rows
                                auto val_a = evaluator.evaluate(sort_key.expression.get(), row_a_data);
                                auto val_b = evaluator.evaluate(sort_key.expression.get(), row_b_data);
                                
                                // Compare values
                                bool less_than = false;
                                bool greater_than = false;
                                
                                // Handle different value types
                                if (std::holds_alternative<int64_t>(val_a) && std::holds_alternative<int64_t>(val_b)) {
                                    int64_t a = std::get<int64_t>(val_a);
                                    int64_t b = std::get<int64_t>(val_b);
                                    less_than = (a < b);
                                    greater_than = (a > b);
                                } else if (std::holds_alternative<double>(val_a) && std::holds_alternative<double>(val_b)) {
                                    double a = std::get<double>(val_a);
                                    double b = std::get<double>(val_b);
                                    less_than = (a < b);
                                    greater_than = (a > b);
                                } else if (std::holds_alternative<std::string>(val_a) && std::holds_alternative<std::string>(val_b)) {
                                    const auto& a = std::get<std::string>(val_a);
                                    const auto& b = std::get<std::string>(val_b);
                                    less_than = (a < b);
                                    greater_than = (a > b);
                                }
                                
                                // Return based on sort direction
                                if (less_than) {
                                    return (sort_key.direction == query::SortDirection::ASC);
                                } else if (greater_than) {
                                    return (sort_key.direction == query::SortDirection::DESC);
                                }
                                // If equal, continue to next sort key
                            }
                            return false;  // All sort keys are equal
                        }
                    );
                }
            }
            
            // Handle LIMIT and OFFSET
            int64_t offset = select_stmt->offset;  // Default 0
            int64_t limit = select_stmt->limit;     // Default -1 (no limit)
            
            // Apply OFFSET first
            if (offset > 0 && static_cast<size_t>(offset) < rows.size()) {
                std::vector<std::vector<std::string>> offset_rows(
                    rows.begin() + offset,
                    rows.end()
                );
                rows = offset_rows;
            } else if (offset > 0) {
                rows.clear();  // All rows skipped
            }
            
            // Apply LIMIT
            if (limit > 0 && static_cast<size_t>(limit) < rows.size()) {
                rows.erase(rows.begin() + limit, rows.end());
            }
            
            // Create result with in-memory data
            return std::make_unique<EngineQueryResult>(rows, col_names);
        }
        
        return nullptr;
    }
    
    throw std::runtime_error("Unknown statement type");
}

std::vector<std::string> Database::list_tables() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : tables_) {
        names.push_back(name);
    }
    return names;
}

Schema Database::get_schema(const std::string& table_name) const {
    auto it = tables_.find(table_name);
    if (it == tables_.end()) {
        throw std::runtime_error("Table not found: " + table_name);
    }
    return it->second->get_schema();
}

void Database::close() {
    is_open_ = false;
}

}  // namespace lyradb
