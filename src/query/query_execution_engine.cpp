#include "lyradb/query_execution_engine.h"
#include "lyradb/database.h"
#include "lyradb/query_executor.h"
#include "lyradb/expression_evaluator.h"
#include "lyradb/sql_parser.h"
#include "lyradb/query_plan.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace lyradb {

QueryExecutionEngine::QueryExecutionEngine(Database* database)
    : database_(database), 
      batch_size_(1024), 
      simd_enabled_(true) {
    
    if (!database_) {
        throw std::runtime_error("Database pointer cannot be null");
    }
    
    // Initialize components
    parser_ = std::make_unique<query::SqlParser>();
    optimizer_ = std::make_unique<plan::QueryOptimizer>();
    executor_ = std::make_unique<QueryExecutor>(database_);
    evaluator_ = std::make_unique<ExpressionEvaluator>();
    
    // Configure executor
    executor_->set_batch_size(batch_size_);
    executor_->set_simd_enabled(simd_enabled_);
}

QueryExecutionEngine::~QueryExecutionEngine() = default;

QueryExecutionEngine::QueryResult QueryExecutionEngine::execute(const std::string& sql) {
    auto start_time = std::chrono::high_resolution_clock::now();
    QueryResult result;
    result.rows_processed = 0;
    
    try {
        // Validate input
        if (sql.empty()) {
            throw std::runtime_error("Query string cannot be empty");
        }
        
        if (!parser_) {
            throw std::runtime_error("Parser not initialized");
        }
        
        if (!optimizer_) {
            throw std::runtime_error("Optimizer not initialized");
        }
        
        if (!executor_) {
            throw std::runtime_error("Executor not initialized");
        }
        
        // Step 1: Parse SQL
        std::unique_ptr<query::SelectStatement> stmt;
        try {
            stmt = parser_->parse_select_statement(sql);
        } catch (const std::exception& e) {
            throw std::runtime_error("Parse error: " + std::string(e.what()));
        }
        
        if (!stmt) {
            throw std::runtime_error("Failed to parse SQL query - parser returned null");
        }
        
        // Step 2: Validate query structure
        try {
            validate_query(*stmt);
        } catch (const std::exception& e) {
            throw std::runtime_error("Query validation error: " + std::string(e.what()));
        }
        
        // Step 3: Optimize query plan
        std::unique_ptr<plan::QueryPlan> plan;
        try {
            // Cast from lyradb::query::SelectStatement to lyradb::SelectStatement
            plan = optimizer_->optimize(*reinterpret_cast<const lyradb::SelectStatement*>(stmt.get()));
        } catch (const std::exception& e) {
            throw std::runtime_error("Optimization error: " + std::string(e.what()));
        }
        
        if (!plan) {
            throw std::runtime_error("Failed to optimize query plan - optimizer returned null");
        }
        
        // Store execution plan for diagnostics
        try {
            last_execution_plan_ = plan->to_string();
            result.execution_plan = last_execution_plan_;
        } catch (...) {
            // If plan-to-string fails, just continue
            result.execution_plan = "[Plan unavailable]";
        }
        
        // Step 4: Execute query
        try {
            executor_->execute(*reinterpret_cast<const lyradb::QueryPlan*>(plan.get()));
        } catch (const std::exception& e) {
            throw std::runtime_error("Execution error: " + std::string(e.what()));
        }
        
        // Step 5: Get statistics from executor
        try {
            auto stats_str = executor_->get_stats();
            result.rows_processed = plan->estimated_rows();
        } catch (...) {
            result.rows_processed = 0;
        }
        
        result.rows_returned = std::min(result.rows_processed, 1000ULL);  // Sample output
        
        // Step 6: Materialize results
        // Extract column names from the SELECT statement
        result.column_names.clear();
        
        // If there are selected columns, use them; otherwise use *
        if (!stmt->select_list.empty()) {
            // For simple column references, extract the column names
            for (const auto& expr : stmt->select_list) {
                // This is a simplified approach - assumes ColumnRefExpr
                // In a full implementation, would recursively extract column names
                result.column_names.push_back("col_" + std::to_string(result.column_names.size()));
            }
        } else {
            // SELECT * case - use generic column names
            result.column_names.push_back("*");
        }
        
        // For Phase 5.5, return sample materialized results
        // Full implementation would deserialize from ExecutionContext binary data
        if (result.rows_processed > 0) {
            // Generate sample rows based on result count
            for (uint64_t i = 0; i < std::min(result.rows_returned, 10ULL); ++i) {
                std::vector<std::string> row;
                for (size_t j = 0; j < result.column_names.size(); ++j) {
                    row.push_back("row_" + std::to_string(i) + "_col_" + std::to_string(j));
                }
                result.rows.push_back(row);
            }
        }
        
        // Update statistics
        stats_.total_queries_executed++;
        stats_.total_rows_processed += result.rows_processed;
        stats_.last_error = "";
        
    } catch (const std::exception& e) {
        stats_.last_error = e.what();
        result.execution_plan = "";
        result.rows_processed = 0;
        result.rows_returned = 0;
        // Note: Throwing exceptions is handled by caller
        throw;
        throw;
    }
    
    // Record execution time
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    result.execution_time_ms = duration.count() / 1000.0;
    stats_.total_execution_time_ms += result.execution_time_ms;
    
    return result;
}

std::string QueryExecutionEngine::QueryResult::to_csv() const {
    std::ostringstream oss;
    
    // Header row
    for (size_t i = 0; i < column_names.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\"" << column_names[i] << "\"";
    }
    oss << "\n";
    
    // Data rows
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "\"" << row[i] << "\"";
        }
        oss << "\n";
    }
    
    return oss.str();
}

std::string QueryExecutionEngine::QueryResult::to_json() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"columns\": [";
    for (size_t i = 0; i < column_names.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << "\"" << column_names[i] << "\"";
    }
    oss << "],\n";
    
    oss << "  \"rows\": [\n";
    for (size_t r = 0; r < rows.size(); ++r) {
        if (r > 0) oss << ",\n";
        oss << "    {";
        for (size_t c = 0; c < column_names.size(); ++c) {
            if (c > 0) oss << ", ";
            oss << "\"" << column_names[c] << "\": ";
            
            if (c < rows[r].size()) {
                // Try to parse as number
                try {
                    std::stod(rows[r][c]);
                    oss << rows[r][c];  // It's a number
                } catch (...) {
                    oss << "\"" << rows[r][c] << "\"";  // It's a string
                }
            } else {
                oss << "null";
            }
        }
        oss << "}";
    }
    oss << "\n  ]\n";
    oss << "}\n";
    
    return oss.str();
}

std::string QueryExecutionEngine::QueryResult::to_table() const {
    if (rows.empty() || column_names.empty()) {
        return "No results\n";
    }
    
    std::ostringstream oss;
    
    // Calculate column widths
    std::vector<size_t> col_widths(column_names.size());
    for (size_t i = 0; i < column_names.size(); ++i) {
        col_widths[i] = column_names[i].length();
    }
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size() && i < col_widths.size(); ++i) {
            col_widths[i] = std::max(col_widths[i], row[i].length());
        }
    }
    
    // Print header
    oss << "+";
    for (size_t i = 0; i < column_names.size(); ++i) {
        oss << std::string(col_widths[i] + 2, '-') << "+";
    }
    oss << "\n";
    
    oss << "|";
    for (size_t i = 0; i < column_names.size(); ++i) {
        oss << " " << std::left << std::setw(col_widths[i]) << column_names[i] << " |";
    }
    oss << "\n";
    
    oss << "+";
    for (size_t i = 0; i < column_names.size(); ++i) {
        oss << std::string(col_widths[i] + 2, '-') << "+";
    }
    oss << "\n";
    
    // Print rows
    for (const auto& row : rows) {
        oss << "|";
        for (size_t i = 0; i < column_names.size(); ++i) {
            std::string cell = (i < row.size()) ? row[i] : "";
            oss << " " << std::left << std::setw(col_widths[i]) << cell << " |";
        }
        oss << "\n";
    }
    
    oss << "+";
    for (size_t i = 0; i < column_names.size(); ++i) {
        oss << std::string(col_widths[i] + 2, '-') << "+";
    }
    oss << "\n";
    
    oss << "\n(" << rows.size() << " rows)\n";
    
    return oss.str();
}

std::string QueryExecutionEngine::get_last_execution_plan() const {
    return last_execution_plan_;
}

void QueryExecutionEngine::set_batch_size(size_t size) {
    batch_size_ = size;
    if (executor_) {
        executor_->set_batch_size(size);
    }
}

void QueryExecutionEngine::set_simd_enabled(bool enable) {
    simd_enabled_ = enable;
    if (executor_) {
        executor_->set_simd_enabled(enable);
    }
}

const QueryExecutionEngine::ExecutionStats& QueryExecutionEngine::get_stats() const {
    return stats_;
}

void QueryExecutionEngine::validate_query(const query::SelectStatement& stmt) {
    // Validate FROM clause
    if (stmt.from_table) {
        validate_schema(stmt.from_table->table_name);
    }
}

void QueryExecutionEngine::validate_schema(const std::string& table_name) {
    if (!database_) {
        throw std::runtime_error("Database not available");
    }
    
    auto table = database_->get_table(table_name);
    if (!table) {
        throw std::runtime_error("Table not found: " + table_name);
    }
}

} // namespace lyradb
