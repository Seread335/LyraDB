#include "lyradb/query_executor.h"
#include "lyradb/query_plan.h"
#include "lyradb/query_result.h"
#include "lyradb/database.h"
#include "lyradb/table.h"
#include "lyradb/column.h"
#include "lyradb/composite_query_optimizer.h"
#include "lyradb/simple_query_optimizer.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <unordered_set>

namespace lyradb {

QueryExecutor::QueryExecutor(Database* database)
    : batch_size_(1024), simd_enabled_(true), 
      rows_processed_(0), batches_processed_(0), database_(database) {
}

QueryExecutor::~QueryExecutor() {
}

void QueryExecutor::execute(const QueryPlan& plan) {
    // Create execution context
    ExecutionContext ctx;
    ctx.row_count = 0;
    
    // Execute the query plan starting from root
    // In a real implementation, we'd traverse the plan tree
    // For now, just track statistics
    
    rows_processed_ += ctx.row_count;
    batches_processed_++;
}

void QueryExecutor::set_batch_size(size_t size) {
    batch_size_ = std::max(size_t(64), std::min(size, size_t(8192)));
}

void QueryExecutor::set_simd_enabled(bool enable) {
    simd_enabled_ = enable;
}

std::string QueryExecutor::get_stats() const {
    std::string stats;
    stats += "Query Executor Statistics:\n";
    stats += "  Rows Processed: " + std::to_string(rows_processed_) + "\n";
    stats += "  Batches Processed: " + std::to_string(batches_processed_) + "\n";
    stats += "  Batch Size: " + std::to_string(batch_size_) + "\n";
    stats += "  SIMD Enabled: " + std::string(simd_enabled_ ? "Yes" : "No") + "\n";
    
    if (batches_processed_ > 0) {
        double avg_batch_size = static_cast<double>(rows_processed_) / batches_processed_;
        stats += "  Average Batch Size: " + std::to_string(static_cast<uint64_t>(avg_batch_size)) + "\n";
    }
    
    return stats;
}

size_t QueryExecutor::execute_node(const plan::PlanNode* node, ExecutionContext& ctx) {
    if (!node) {
        return 0;
    }
    
    plan::NodeType type = node->type();
    
    switch (type) {
        case plan::NodeType::TableScan:
            return execute_scan(node, ctx);
        case plan::NodeType::Filter:
            return execute_filter(node, ctx);
        case plan::NodeType::Project:
            return execute_project(node, ctx);
        case plan::NodeType::Join:
            return execute_join(node, ctx);
        case plan::NodeType::Aggregate:
            return execute_aggregate(node, ctx);
        case plan::NodeType::Sort:
            return execute_sort(node, ctx);
        case plan::NodeType::Limit:
            return execute_limit(node, ctx);
        default:
            return 0;
    }
}

size_t QueryExecutor::execute_scan(const plan::PlanNode* node, ExecutionContext& ctx) {
    // Cast to ScanNode to get table information
    const plan::ScanNode* scan = dynamic_cast<const plan::ScanNode*>(node);
    if (!scan || !database_) {
        ctx.row_count = 0;
        return 0;
    }
    
    // Get table from database
    auto table = database_->get_table(scan->table_name());
    if (!table) {
        ctx.row_count = 0;
        return 0;
    }
    
    // Clear previous batch data
    ctx.data.clear();
    ctx.column_index.clear();
    
    size_t num_rows = table->row_count();
    size_t num_cols = table->column_count();
    
    if (num_rows == 0) {
        ctx.row_count = 0;
        return 0;
    }
    
    // Read rows in batches up to batch_size_
    size_t batch_rows = std::min(batch_size_, num_rows);
    
    // Initialize column data vectors (one per column)
    ctx.data.resize(num_cols);
    
    // Build column index map
    const Schema& schema = table->get_schema();
    for (size_t i = 0; i < num_cols; i++) {
        const auto& col_def = schema.get_column(i);
        ctx.column_index[col_def.name] = i;
    }
    
    // Read data from each column
    for (size_t col_idx = 0; col_idx < num_cols; col_idx++) {
        auto column = table->get_column(col_idx);
        if (!column) {
            ctx.row_count = 0;
            return 0;
        }
        
        // Serialize column data to bytes
        // For now, use placeholder implementation
        // In real implementation, would read actual binary data
        ctx.data[col_idx].reserve(batch_rows * 8);  // Assume ~8 bytes per value
    }
    
    ctx.row_count = batch_rows;
    rows_processed_ += batch_rows;
    batches_processed_++;
    
    return batch_rows;
}

size_t QueryExecutor::execute_filter(const plan::PlanNode* node, ExecutionContext& ctx) {
    // Apply WHERE clause with predicate evaluation using Phase 6 optimization
    const plan::FilterNode* filter = dynamic_cast<const plan::FilterNode*>(node);
    if (!filter || ctx.row_count == 0) {
        return ctx.row_count;
    }
    
    // First execute child to get batch data
    auto child = filter->child();
    if (!child) {
        ctx.row_count = 0;
        return 0;
    }
    
    size_t input_rows = execute_node(child, ctx);
    if (input_rows == 0) {
        ctx.row_count = 0;
        return 0;
    }
    
    // Parse predicate condition (simple cases: column op value)
    const std::string& condition = filter->condition();
    std::vector<bool> row_mask(input_rows, false);
    
    // ========================================================================
    // PHASE 6: SimpleQueryOptimizer Integration
    // ========================================================================
    // Use lightweight optimizer to determine best execution strategy
    // Strategies: FULL_SCAN, INDEX_SINGLE, INDEX_RANGE, INDEX_INTERSECTION, INDEX_UNION
    
    std::cout << "\n[PHASE 6] Execute filter - Applying SimpleQueryOptimizer\n";
    std::cout << "[PHASE 6] Condition: " << condition << "\n";
    std::cout << "[PHASE 6] Input rows: " << input_rows << "\n";
    
    // Create optimizer and register available indexes
    integration::SimpleQueryOptimizer optimizer;
    optimizer.register_index("idx_age", "age");
    optimizer.register_index("idx_country", "country");
    optimizer.register_index("idx_salary", "salary");
    optimizer.register_index("idx_status", "status");
    
    // Get optimization plan
    integration::SimpleQueryOptimizer::Plan opt_plan = optimizer.optimize(
        condition,
        input_rows,
        {"idx_age", "idx_country", "idx_salary", "idx_status"}
    );
    
    std::cout << "[PHASE 6] Strategy: " << 
        (opt_plan.strategy == integration::SimpleQueryOptimizer::Strategy::FULL_SCAN ? "FULL_SCAN" :
         opt_plan.strategy == integration::SimpleQueryOptimizer::Strategy::INDEX_SINGLE ? "INDEX_SINGLE" :
         opt_plan.strategy == integration::SimpleQueryOptimizer::Strategy::INDEX_RANGE ? "INDEX_RANGE" :
         opt_plan.strategy == integration::SimpleQueryOptimizer::Strategy::INDEX_INTERSECTION ? "INDEX_INTERSECTION" :
         "INDEX_UNION") << "\n";
    std::cout << "[PHASE 6] Predicted speedup: " << opt_plan.predicted_speedup << "x\n";
    std::cout << "[PHASE 6] Explanation: " << opt_plan.explanation << "\n";
    
    // ========================================================================
    // FULL SCAN: Evaluate all rows (O(n) complexity)
    // ========================================================================
    
    // Extract column name, operator, and value from condition
    // Format: "column_name op value" (e.g., "age > 18", "active = true")
    
    size_t op_pos = condition.find_first_of("<>=!~");
    if (op_pos == std::string::npos || op_pos == 0) {
        // Invalid condition, pass all rows
        return input_rows;
    }
    
    std::string col_name = condition.substr(0, op_pos);
    // Trim whitespace
    col_name.erase(col_name.find_last_not_of(" \t\n\r\f\v") + 1);
    
    // Find operator
    std::string op;
    size_t val_start = op_pos;
    if (condition[op_pos] == '<' || condition[op_pos] == '>') {
        if (op_pos + 1 < condition.length() && condition[op_pos + 1] == '=') {
            op = condition.substr(op_pos, 2);
            val_start = op_pos + 2;
        } else {
            op = condition.substr(op_pos, 1);
            val_start = op_pos + 1;
        }
    } else if (condition[op_pos] == '=' || condition[op_pos] == '!') {
        if (op_pos + 1 < condition.length() && condition[op_pos + 1] == '=') {
            op = condition.substr(op_pos, 2);
            val_start = op_pos + 2;
        } else {
            op = condition.substr(op_pos, 1);
            val_start = op_pos + 1;
        }
    }
    
    // Extract value
    std::string value_str = condition.substr(val_start);
    value_str.erase(0, value_str.find_first_not_of(" \t\n\r\f\v"));
    value_str.erase(value_str.find_last_not_of(" \t\n\r\f\v") + 1);
    
    // Get column index
    auto col_it = ctx.column_index.find(col_name);
    if (col_it == ctx.column_index.end() || col_it->second >= ctx.data.size()) {
        // Column not found, pass all rows
        return input_rows;
    }
    
    size_t col_idx = col_it->second;
    const auto& col_data = ctx.data[col_idx];
    
    // Evaluate predicate for each row (simple approach - parse as numeric for now)
    try {
        double threshold = std::stod(value_str);
        
        // Sample evaluation assuming 8-byte double values
        size_t bytes_per_row = col_data.size() / input_rows;
        if (bytes_per_row < 8) bytes_per_row = 8;
        
        int matched_rows = 0;
        for (size_t i = 0; i < input_rows; ++i) {
            size_t offset = i * bytes_per_row;
            if (offset + 8 > col_data.size()) break;
            
            // Interpret as double
            double col_value = 0.0;
            std::memcpy(&col_value, col_data.data() + offset, 8);
            
            bool matches = false;
            if (op == ">") {
                matches = col_value > threshold;
            } else if (op == "<") {
                matches = col_value < threshold;
            } else if (op == ">=") {
                matches = col_value >= threshold;
            } else if (op == "<=") {
                matches = col_value <= threshold;
            } else if (op == "=" || op == "==") {
                matches = std::abs(col_value - threshold) < 1e-9;
            } else if (op == "!=" || op == "<>") {
                matches = std::abs(col_value - threshold) >= 1e-9;
            }
            
            row_mask[i] = matches;
            if (matches) matched_rows++;
        }
        
        // Copy filtered rows back to context
        std::vector<std::vector<uint8_t>> filtered_data;
        for (size_t col = 0; col < ctx.data.size(); ++col) {
            std::vector<uint8_t> filtered_col;
            size_t bytes_per_row = ctx.data[col].size() / input_rows;
            if (bytes_per_row == 0) bytes_per_row = 1;
            
            for (size_t i = 0; i < input_rows; ++i) {
                if (row_mask[i]) {
                    size_t offset = i * bytes_per_row;
                    size_t end = std::min(offset + bytes_per_row, ctx.data[col].size());
                    filtered_col.insert(filtered_col.end(),
                        ctx.data[col].begin() + offset,
                        ctx.data[col].begin() + end);
                }
            }
            filtered_data.push_back(filtered_col);
        }
        
        ctx.data = filtered_data;
        ctx.row_count = matched_rows;
        rows_processed_ += matched_rows;
        
        return matched_rows;
        
    } catch (const std::exception& e) {
        // If numeric parsing fails, pass all rows
        return input_rows;
    }
}

size_t QueryExecutor::execute_project(const plan::PlanNode* node, ExecutionContext& ctx) {
    // Select specific columns from the result set
    const plan::ProjectNode* project = dynamic_cast<const plan::ProjectNode*>(node);
    if (!project) {
        return ctx.row_count;
    }
    
    // First execute child to get batch data
    auto child = project->child();
    if (!child) {
        ctx.row_count = 0;
        return 0;
    }
    
    size_t input_rows = execute_node(child, ctx);
    if (input_rows == 0) {
        ctx.row_count = 0;
        return 0;
    }
    
    // Get requested columns
    const auto& columns = project->columns();
    if (columns.empty()) {
        // No columns selected, return 0 rows
        ctx.data.clear();
        ctx.row_count = 0;
        return 0;
    }
    
    // Build new column data with only requested columns
    std::vector<std::vector<uint8_t>> projected_data;
    
    for (const auto& col_name : columns) {
        // Find column in current context
        auto col_it = ctx.column_index.find(col_name);
        if (col_it != ctx.column_index.end() && col_it->second < ctx.data.size()) {
            size_t col_idx = col_it->second;
            projected_data.push_back(ctx.data[col_idx]);
        } else {
            // Column not found, create empty column data
            size_t bytes_per_row = ctx.data.empty() ? 8 : (ctx.data[0].size() / input_rows);
            std::vector<uint8_t> empty_col(bytes_per_row * input_rows, 0);
            projected_data.push_back(empty_col);
        }
    }
    
    // Rebuild column index map for projected columns
    std::unordered_map<std::string, size_t> new_col_index;
    for (size_t i = 0; i < columns.size(); ++i) {
        new_col_index[columns[i]] = i;
    }
    
    ctx.data = projected_data;
    ctx.column_index = new_col_index;
    
    // Project doesn't change row count, only column count
    return input_rows;
}

size_t QueryExecutor::execute_join(const plan::PlanNode* node, ExecutionContext& ctx) {
    // Execute join operation (INNER, LEFT, RIGHT, FULL)
    const plan::JoinNode* join = dynamic_cast<const plan::JoinNode*>(node);
    if (!join) {
        ctx.row_count = 0;
        return 0;
    }
    
    // Execute both children to get batch data
    auto left_child = join->left();
    auto right_child = join->right();
    
    if (!left_child || !right_child) {
        ctx.row_count = 0;
        return 0;
    }
    
    // Build left side
    ExecutionContext left_ctx;
    left_ctx.row_count = 0;
    size_t left_rows = execute_node(left_child, left_ctx);
    if (left_rows == 0) {
        ctx.row_count = 0;
        return 0;
    }
    
    // Build right side
    ExecutionContext right_ctx;
    right_ctx.row_count = 0;
    size_t right_rows = execute_node(right_child, right_ctx);
    if (right_rows == 0) {
        // For INNER join, no output; for LEFT join, return left side
        if (join->algorithm() == plan::JoinAlgorithm::HASH_JOIN) {
            ctx = left_ctx;
            return left_rows;
        }
        ctx.row_count = 0;
        return 0;
    }
    
    // Parse join condition to extract key columns
    const std::string& condition = join->condition();
    // Simple parse: "left_col = right_col"
    size_t eq_pos = condition.find('=');
    if (eq_pos == std::string::npos) {
        ctx.row_count = 0;
        return 0;
    }
    
    std::string left_key = condition.substr(0, eq_pos);
    std::string right_key = condition.substr(eq_pos + 1);
    
    // Trim whitespace
    left_key.erase(left_key.find_last_not_of(" \t\n\r\f\v") + 1);
    left_key.erase(0, left_key.find_first_not_of(" \t\n\r\f\v"));
    right_key.erase(0, right_key.find_first_not_of(" \t\n\r\f\v"));
    right_key.erase(right_key.find_last_not_of(" \t\n\r\f\v") + 1);
    
    // Find key columns in left and right
    auto left_key_it = left_ctx.column_index.find(left_key);
    auto right_key_it = right_ctx.column_index.find(right_key);
    
    if (left_key_it == left_ctx.column_index.end() || 
        right_key_it == right_ctx.column_index.end()) {
        ctx.row_count = 0;
        return 0;
    }
    
    size_t left_key_idx = left_key_it->second;
    size_t right_key_idx = right_key_it->second;
    
    // Simple nested loop join (not optimal for large tables, but straightforward)
    std::vector<std::vector<uint8_t>> join_data;
    std::vector<std::pair<size_t, size_t>> matching_pairs;  // (left_row, right_row)
    
    // Initialize join result columns (all left columns + all right columns)
    for (auto& col : left_ctx.data) {
        join_data.push_back(std::vector<uint8_t>());
    }
    for (auto& col : right_ctx.data) {
        join_data.push_back(std::vector<uint8_t>());
    }
    
    size_t left_bytes_per_row = left_ctx.data[0].size() / left_rows;
    size_t right_bytes_per_row = right_ctx.data[0].size() / right_rows;
    if (left_bytes_per_row == 0) left_bytes_per_row = 8;
    if (right_bytes_per_row == 0) right_bytes_per_row = 8;
    
    // Extract left key values
    std::vector<double> left_key_vals(left_rows);
    const auto& left_key_data = left_ctx.data[left_key_idx];
    for (size_t i = 0; i < left_rows; ++i) {
        size_t offset = i * left_bytes_per_row;
        if (offset + 8 <= left_key_data.size()) {
            std::memcpy(&left_key_vals[i], left_key_data.data() + offset, 8);
        }
    }
    
    // Extract right key values
    std::vector<double> right_key_vals(right_rows);
    const auto& right_key_data = right_ctx.data[right_key_idx];
    for (size_t i = 0; i < right_rows; ++i) {
        size_t offset = i * right_bytes_per_row;
        if (offset + 8 <= right_key_data.size()) {
            std::memcpy(&right_key_vals[i], right_key_data.data() + offset, 8);
        }
    }
    
    // Nested loop join: find matching rows
    for (size_t l = 0; l < left_rows; ++l) {
        for (size_t r = 0; r < right_rows; ++r) {
            if (std::abs(left_key_vals[l] - right_key_vals[r]) < 1e-9) {
                matching_pairs.push_back({l, r});
            }
        }
    }
    
    // Build joined result rows
    size_t joined_rows = matching_pairs.size();
    
    // Copy left columns
    for (size_t col = 0; col < left_ctx.data.size(); ++col) {
        const auto& src_col = left_ctx.data[col];
        auto& dst_col = join_data[col];
        
        for (auto& pair : matching_pairs) {
            size_t offset = pair.first * left_bytes_per_row;
            size_t end = std::min(offset + left_bytes_per_row, src_col.size());
            dst_col.insert(dst_col.end(), src_col.begin() + offset, src_col.begin() + end);
        }
    }
    
    // Copy right columns
    for (size_t col = 0; col < right_ctx.data.size(); ++col) {
        const auto& src_col = right_ctx.data[col];
        auto& dst_col = join_data[left_ctx.data.size() + col];
        
        for (auto& pair : matching_pairs) {
            size_t offset = pair.second * right_bytes_per_row;
            size_t end = std::min(offset + right_bytes_per_row, src_col.size());
            dst_col.insert(dst_col.end(), src_col.begin() + offset, src_col.begin() + end);
        }
    }
    
    // Build merged column index
    std::unordered_map<std::string, size_t> joined_col_index;
    size_t col_idx = 0;
    for (auto& pair : left_ctx.column_index) {
        joined_col_index[pair.first] = col_idx++;
    }
    for (auto& pair : right_ctx.column_index) {
        // Prefix right columns to avoid conflicts
        joined_col_index["r." + pair.first] = col_idx++;
    }
    
    ctx.data = join_data;
    ctx.column_index = joined_col_index;
    ctx.row_count = joined_rows;
    rows_processed_ += joined_rows;
    
    return joined_rows;
}

size_t QueryExecutor::execute_aggregate(const plan::PlanNode* node, ExecutionContext& ctx) {
    // Execute aggregation (SUM, COUNT, AVG, MIN, MAX)
    const plan::AggregateNode* agg = dynamic_cast<const plan::AggregateNode*>(node);
    if (!agg) {
        ctx.row_count = 0;
        return 0;
    }
    
    // First execute child to get batch data
    auto child = agg->child();
    if (!child) {
        ctx.row_count = 0;
        return 0;
    }
    
    size_t input_rows = execute_node(child, ctx);
    if (input_rows == 0) {
        // Empty input - aggregates should return 0/NULL values
        ctx.row_count = 1;
        ctx.data.clear();
        for (size_t i = 0; i < 10; ++i) {  // Reserve space for aggregate results
            ctx.data.push_back(std::vector<uint8_t>(8, 0));
        }
        return 1;
    }
    
    // Get aggregate specifications
    const auto& agg_exprs = agg->aggregate_exprs();
    const auto& group_by_cols = agg->group_by_cols();
    
    // For simple aggregates without GROUP BY
    if (group_by_cols.empty()) {
        // Calculate aggregates for entire batch
        std::vector<std::vector<uint8_t>> agg_data;
        
        // Parse each aggregate expression (e.g., "SUM(amount)", "COUNT(*)")
        for (const auto& expr : agg_exprs) {
            std::vector<uint8_t> agg_result(8, 0);  // One result per aggregate
            
            // Simple parsing: extract function name and column
            size_t paren_pos = expr.find('(');
            if (paren_pos == std::string::npos) {
                agg_data.push_back(agg_result);
                continue;
            }
            
            std::string func_name = expr.substr(0, paren_pos);
            func_name.erase(0, func_name.find_first_not_of(" \t\n\r\f\v"));
            func_name.erase(func_name.find_last_not_of(" \t\n\r\f\v") + 1);
            
            size_t col_start = paren_pos + 1;
            size_t col_end = expr.find(')', col_start);
            if (col_end == std::string::npos) {
                agg_data.push_back(agg_result);
                continue;
            }
            
            std::string col_name = expr.substr(col_start, col_end - col_start);
            col_name.erase(0, col_name.find_first_not_of(" \t\n\r\f\v"));
            col_name.erase(col_name.find_last_not_of(" \t\n\r\f\v") + 1);
            
            double agg_value = 0.0;
            
            if (func_name == "COUNT" && col_name == "*") {
                // COUNT(*) = number of rows
                agg_value = static_cast<double>(input_rows);
            } else if (col_name != "*" && ctx.column_index.find(col_name) != ctx.column_index.end()) {
                // Get column data
                size_t col_idx = ctx.column_index[col_name];
                if (col_idx < ctx.data.size()) {
                    const auto& col_data = ctx.data[col_idx];
                    size_t bytes_per_row = col_data.size() / input_rows;
                    if (bytes_per_row == 0) bytes_per_row = 8;
                    
                    double sum = 0.0, min_val = 1e9, max_val = -1e9;
                    int count = 0;
                    
                    for (size_t i = 0; i < input_rows; ++i) {
                        size_t offset = i * bytes_per_row;
                        if (offset + 8 <= col_data.size()) {
                            double val = 0.0;
                            std::memcpy(&val, col_data.data() + offset, 8);
                            
                            sum += val;
                            min_val = std::min(min_val, val);
                            max_val = std::max(max_val, val);
                            count++;
                        }
                    }
                    
                    if (func_name == "SUM") {
                        agg_value = sum;
                    } else if (func_name == "COUNT") {
                        agg_value = static_cast<double>(count);
                    } else if (func_name == "AVG") {
                        agg_value = count > 0 ? (sum / count) : 0.0;
                    } else if (func_name == "MIN") {
                        agg_value = min_val == 1e9 ? 0.0 : min_val;
                    } else if (func_name == "MAX") {
                        agg_value = max_val == -1e9 ? 0.0 : max_val;
                    }
                }
            }
            
            std::memcpy(agg_result.data(), &agg_value, 8);
            agg_data.push_back(agg_result);
        }
        
        ctx.data = agg_data;
        ctx.row_count = 1;  // Aggregates return single row
        rows_processed_ += 1;
        
        return 1;
    } else {
        // GROUP BY aggregates - simplified implementation
        // For now, return single aggregate row
        std::vector<std::vector<uint8_t>> agg_data;
        for (const auto& expr : agg_exprs) {
            agg_data.push_back(std::vector<uint8_t>(8, 0));
        }
        ctx.data = agg_data;
        ctx.row_count = 1;
        return 1;
    }
}

size_t QueryExecutor::execute_sort(const plan::PlanNode* node, ExecutionContext& ctx) {
    // Sort batch using ORDER BY specification
    const plan::SortNode* sort = dynamic_cast<const plan::SortNode*>(node);
    if (!sort || ctx.row_count == 0) {
        return ctx.row_count;
    }
    
    // First execute child to get batch data
    auto child = sort->child();
    if (!child) {
        ctx.row_count = 0;
        return 0;
    }
    
    size_t input_rows = execute_node(child, ctx);
    if (input_rows == 0) {
        ctx.row_count = 0;
        return 0;
    }
    
    // Get sort specifications
    const auto& sort_keys = sort->sort_keys();
    if (sort_keys.empty() || ctx.data.empty()) {
        return input_rows;
    }
    
    // For now, implement single-key sort on first sort key
    const auto& first_key = sort_keys[0];
    const std::string& sort_col = first_key.column;  // column name
    bool ascending = first_key.ascending;             // sort order (true = ASC)
    
    // Find column index
    auto col_it = ctx.column_index.find(sort_col);
    if (col_it == ctx.column_index.end() || col_it->second >= ctx.data.size()) {
        return input_rows;
    }
    
    size_t sort_col_idx = col_it->second;
    size_t bytes_per_row = ctx.data[sort_col_idx].size() / input_rows;
    if (bytes_per_row == 0) bytes_per_row = 8;
    
    // Create index array for sorting (avoid rearranging actual data until end)
    std::vector<size_t> row_indices(input_rows);
    std::iota(row_indices.begin(), row_indices.end(), 0);
    
    // Sort indices based on column values
    const auto& sort_col_data = ctx.data[sort_col_idx];
    std::sort(row_indices.begin(), row_indices.end(),
        [&sort_col_data, bytes_per_row, ascending](size_t a, size_t b) {
            // Extract double values from column data
            double val_a = 0.0, val_b = 0.0;
            
            size_t offset_a = a * bytes_per_row;
            if (offset_a + 8 <= sort_col_data.size()) {
                std::memcpy(&val_a, sort_col_data.data() + offset_a, 8);
            }
            
            size_t offset_b = b * bytes_per_row;
            if (offset_b + 8 <= sort_col_data.size()) {
                std::memcpy(&val_b, sort_col_data.data() + offset_b, 8);
            }
            
            return ascending ? (val_a < val_b) : (val_a > val_b);
        });
    
    // Reorder all columns based on sorted indices
    std::vector<std::vector<uint8_t>> sorted_data;
    for (auto& col : ctx.data) {
        size_t bytes_per_row_col = col.size() / input_rows;
        if (bytes_per_row_col == 0) bytes_per_row_col = 1;
        
        std::vector<uint8_t> sorted_col;
        sorted_col.reserve(col.size());
        
        for (size_t idx : row_indices) {
            size_t offset = idx * bytes_per_row_col;
            size_t end = std::min(offset + bytes_per_row_col, col.size());
            sorted_col.insert(sorted_col.end(),
                col.begin() + offset,
                col.begin() + end);
        }
        
        sorted_data.push_back(sorted_col);
    }
    
    ctx.data = sorted_data;
    batches_processed_++;
    
    return input_rows;
}

size_t QueryExecutor::execute_limit(const plan::PlanNode* node, ExecutionContext& ctx) {
    // Apply LIMIT and OFFSET to batch data
    const plan::LimitNode* limit = dynamic_cast<const plan::LimitNode*>(node);
    if (!limit) {
        return ctx.row_count;
    }
    
    // First execute child to get batch data
    auto child = limit->child();
    if (!child) {
        ctx.row_count = 0;
        return 0;
    }
    
    size_t input_rows = execute_node(child, ctx);
    if (input_rows == 0) {
        ctx.row_count = 0;
        return 0;
    }
    
    long long offset = limit->offset();
    long long limit_count = limit->limit();
    
    // Calculate actual start and end row indices
    size_t start_row = offset < 0 ? 0 : static_cast<size_t>(offset);
    size_t end_row = input_rows;
    
    if (limit_count >= 0) {
        end_row = std::min(static_cast<size_t>(offset + limit_count), input_rows);
    }
    
    // If start >= input_rows, no rows to return
    if (start_row >= input_rows) {
        ctx.data.clear();
        ctx.row_count = 0;
        return 0;
    }
    
    // Calculate rows per column
    size_t output_rows = end_row - start_row;
    if (ctx.data.empty()) {
        ctx.row_count = output_rows;
        return output_rows;
    }
    
    size_t bytes_per_row = ctx.data[0].size() / input_rows;
    if (bytes_per_row == 0) bytes_per_row = 1;
    
    // Trim columns to limited range
    std::vector<std::vector<uint8_t>> limited_data;
    for (auto& col : ctx.data) {
        size_t start_byte = start_row * bytes_per_row;
        size_t end_byte = end_row * bytes_per_row;
        
        if (start_byte >= col.size()) {
            limited_data.push_back(std::vector<uint8_t>());
        } else {
            end_byte = std::min(end_byte, col.size());
            std::vector<uint8_t> limited_col(
                col.begin() + start_byte,
                col.begin() + end_byte
            );
            limited_data.push_back(limited_col);
        }
    }
    
    ctx.data = limited_data;
    ctx.row_count = output_rows;
    rows_processed_ += output_rows;
    
    return output_rows;
}

std::vector<std::vector<uint8_t>> QueryExecutor::simd_filter(
    const std::vector<std::vector<uint8_t>>& data,
    const std::string& predicate) {
    
    // SIMD-optimized filter implementation
    // Processes 8-16 rows at a time using SIMD instructions
    
    std::vector<std::vector<uint8_t>> result;
    
    // TODO: Implement SIMD filtering
    // - Parse predicate (e.g., "column > 100")
    // - Load 8-16 rows into SIMD register
    // - Execute comparison in parallel
    // - Store matching rows to result
    
    for (const auto& row : data) {
        // Placeholder: return all rows
        result.push_back(row);
    }
    
    return result;
}

std::vector<std::vector<uint8_t>> QueryExecutor::vectorized_sort(
    const std::vector<std::vector<uint8_t>>& data,
    const std::string& sort_key) {
    
    // Vectorized sort for batch of rows
    // Uses adaptive sorting based on data characteristics
    
    auto result = data;
    
    // TODO: Implement vectorized sort
    // - Detect if data is nearly sorted
    // - Use quicksort for random data
    // - Use insertion sort for nearly sorted data
    // - Use parallel sort for large batches
    
    return result;
}

std::vector<std::vector<uint8_t>> QueryExecutor::hash_join(
    const std::vector<std::vector<uint8_t>>& left,
    const std::vector<std::vector<uint8_t>>& right,
    const std::string& join_key) {
    
    // Hash join implementation
    // Phase 1: Build hash table from right input
    // Phase 2: Probe hash table with left input
    
    std::vector<std::vector<uint8_t>> result;
    
    // TODO: Implement hash join
    // - Extract join key values from right input
    // - Build hash table mapping key -> rows
    // - Iterate left input, probe hash table
    // - Output matching row pairs
    
    return result;
}

// ============================================================================
// PHASE 4.3: Indexed Scan Execution
// ============================================================================

size_t QueryExecutor::execute_indexed_scan(
    const std::string& index_name,
    const std::string& column,
    const std::string& predicate,
    ExecutionContext& ctx) {
    
    // PHASE 4.3: Use B-tree index for efficient row lookup
    // 
    // This method implements O(log n + k) indexed scan instead of O(n) full scan
    // where:
    //   n = total rows in table
    //   k = number of matching rows
    //
    // Algorithm:
    // 1. Parse predicate to extract operator and value
    // 2. Look up B-tree index in database or context
    // 3. Perform appropriate query:
    //    - Equality: search(value)
    //    - Range: range_query(min, max)
    //    - Less than: get_less_than(value)
    //    - Greater than: get_greater_than(value)
    // 4. Return matching row IDs/offsets
    //
    // Expected performance:
    // - Full scan: O(n) - scan all rows
    // - Index scan: O(log n + k) - where k = number of matching rows
    // - Speedup: 10x-500x for selective predicates
    
    if (!database_ || column.empty() || predicate.empty()) {
        return 0;  // Invalid parameters, fallback to full scan
    }
    
    std::cout << "[Phase 4.3 INDEXED SCAN] Index: " << index_name 
              << " | Column: " << column << " | Predicate: " << predicate << "\n";
    
    try {
        // Parse predicate to extract operator and value
        // Format: "op value" where op is: =, <, >, <=, >=, !=
        
        size_t first_space = predicate.find(' ');
        if (first_space == std::string::npos) {
            return 0;  // Invalid predicate format
        }
        
        std::string op = predicate.substr(0, first_space);
        std::string value_str = predicate.substr(first_space + 1);
        
        // Trim whitespace from value
        value_str.erase(0, value_str.find_first_not_of(" \t\n\r\f\v"));
        value_str.erase(value_str.find_last_not_of(" \t\n\r\f\v") + 1);
        
        // Phase 4.3.1: Execute B-tree index operations
        // Handle numeric comparisons with type awareness
        
        try {
            // Convert value to integer (most common case for indexed queries)
            int64_t value = std::stoll(value_str);
            
            std::cout << "  [DEBUG] Parsed predicate: op=" << op << ", value=" << value << "\n";
            
            // Phase 4.3.1: B-tree operations ready for real implementation
            // Current placeholder demonstrates the execution flow with logging
            
            std::cout << "  [Phase 4.3.1] === B-TREE INDEX SCAN ===\n";
            std::cout << "  [Phase 4.3.1] Index: " << index_name << "\n";
            std::cout << "  [Phase 4.3.1] Column: " << column << "\n";
            std::cout << "  [Phase 4.3.1] Operator: " << op << "\n";
            std::cout << "  [Phase 4.3.1] Value: " << value << "\n";
            
            // When actual index_manager is accessible via ctx or database parameter:
            // std::vector<uint32_t> matching_rows;
            //
            // if (op == "=") {
            //     matching_rows = database->index_manager()->search_index(column, value);
            //     std::cout << "  [Phase 4.3.1] Point lookup (=): Found " << matching_rows.size() 
            //               << " rows in O(log n)\n";
            // } else if (op == "<") {
            //     matching_rows = database->index_manager()->range_scan_less_than(column, value);
            //     std::cout << "  [Phase 4.3.1] Range query (<): Found " << matching_rows.size() 
            //               << " rows in O(log n + k)\n";
            // } else if (op == ">") {
            //     matching_rows = database->index_manager()->range_scan_greater_than(column, value);
            //     std::cout << "  [Phase 4.3.1] Range query (>): Found " << matching_rows.size() 
            //               << " rows in O(log n + k)\n";
            // } else if (op == "<=" || op == ">=" || op == "!=") {
            //     // Handle inclusive range queries
            //     // Implementation similar to above
            // }
            //
            // Log expected complexity improvement
            std::cout << "  [Phase 4.3.1] Expected complexity: O(log n + k) vs O(n) full scan\n";
            std::cout << "  [Phase 4.3.1] Ready for integration with IndexManager\n";
            
            // For now: indicate that index operations WOULD be executed
            // Return 0 to signal full scan fallback (will be enabled when index_manager accessible)
            return 0;
            
        } catch (const std::exception& e) {
            // Could not parse value as integer
            std::cerr << "[Phase 4.3] Failed to parse predicate value: " << value_str << "\n";
            return 0;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[Phase 4.3] Error in indexed scan: " << e.what() << "\n";
        return 0;
    }
}

size_t QueryExecutor::execute_composite_indexed_scan(
    const std::string& index_name,
    const std::vector<std::string>& predicates,
    ExecutionContext& ctx) {
    
    // PHASE 4.3: Composite index scan for AND predicates
    //
    // Handles multi-column predicates efficiently:
    // Example: age > 18 AND country = 'USA'
    // 
    // Strategies:
    // 1. Composite Index (if available):
    //    - Single B-tree on (age, country)
    //    - Traversal: O(log n + k)
    //    - Most efficient
    //
    // 2. Index Intersection (multiple single-column indexes):
    //    - First index scan: O(log n + k1)
    //    - Second index scan: O(log n + k2)
    //    - Intersection: O(min(k1, k2))
    //    - Efficient if indexes have low selectivity
    //
    // 3. Fallback to Full Scan:
    //    - All predicates: O(n)
    //    - When no indexes available
    //
    // Algorithm:
    // 1. Parse each predicate
    // 2. Check if composite index available
    // 3. If yes: execute single range query
    // 4. If no: scan each index separately, intersect results
    // 5. Return count of matching rows
    
    if (!database_ || predicates.empty()) {
        return 0;
    }
    
    std::cout << "[Phase 4.3 COMPOSITE SCAN] Index: " << index_name 
              << " | Predicates: " << predicates.size() << "\n";
    
    try {
        // Log all predicates
        for (size_t i = 0; i < predicates.size(); ++i) {
            std::cout << "  [" << i + 1 << "] " << predicates[i] << "\n";
        }
        
        // Parse predicates to understand structure
        std::vector<std::pair<std::string, int64_t>> parsed;
        
        for (const auto& pred : predicates) {
            size_t space_pos = pred.find(' ');
            if (space_pos == std::string::npos) {
                continue;  // Skip malformed predicates
            }
            
            std::string op = pred.substr(0, space_pos);
            std::string val_str = pred.substr(space_pos + 1);
            val_str.erase(0, val_str.find_first_not_of(" \t\n\r\f\v"));
            val_str.erase(val_str.find_last_not_of(" \t\n\r\f\v") + 1);
            
            try {
                int64_t value = std::stoll(val_str);
                parsed.push_back({op, value});
            } catch (const std::exception&) {
                // Skip non-numeric predicates for now
                continue;
            }
        }
        
        // Composite index strategy decision
        // 
        // PSEUDO-CODE for Phase 4.3 composite scan:
        //
        // if (composite_index_exists(index_name)) {
        //     // Strategy 1: Use composite index
        //     // Execute single B-tree traversal with combined predicate
        //     // Example: btree_index->range_query(age_min, age_max, country)
        //     // Efficiency: O(log n + k)
        //     auto result = composite_index->execute_composite_query(predicates);
        //     return result.size();
        // }
        // else if (single_column_indexes_available(predicates)) {
        //     // Strategy 2: Index intersection
        //     // Execute each index separately, intersect results
        //     std::vector<std::set<size_t>> index_results;
        //     for (each predicate) {
        //         auto idx_result = btree_index->execute_predicate(predicate);
        //         index_results.push_back(idx_result);
        //     }
        //     auto intersected = intersect_sets(index_results);
        //     return intersected.size();
        // }
        // else {
        //     // Fallback to full scan
        //     return 0;
        // }
        
        std::cout << "  [INFO] Parsed " << parsed.size() << " numeric predicates\n";
        
        if (parsed.size() < 2) {
            std::cout << "  [WARNING] Less than 2 predicates found, falling back to full scan\n";
        } else {
            std::cout << "  [INFO] Composite scan would use index intersection strategy\n";
            std::cout << "  [INFO] Expected O(log n + k1 + k2) vs O(n) full scan\n";
        }
        
        // Phase 4.3: Return 0 to fall back to full scan
        // Next iteration will implement actual index lookups
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "[Phase 4.3] Error in composite indexed scan: " << e.what() << "\n";
        return 0;
    }
}

size_t QueryExecutor::route_filter_execution(
    const std::string& condition,
    const std::string& table_name,
    size_t input_rows,
    ExecutionContext& ctx) {
    
    // PHASE 4.3: Route filter to indexed or full scan based on optimizer
    //
    // Decision Flow:
    // 1. Get optimization recommendation from Phase 4.2
    // 2. IF recommend_index:
    //    a. Check if index exists
    //    b. Execute indexed scan
    //    c. Fallback to full scan if index unavailable
    // 3. ELSE:
    //    - Execute full table scan (faster than index overhead)
    //
    // Returns:
    // - > 0: Indexed scan executed, return filtered row count
    // - = 0: No indexed scan possible, fallback to full scan
    
    if (condition.empty() || input_rows == 0) {
        return 0;
    }
    
    try {
        // Phase 4.2 integration: Get optimization decision
        CompositeQueryOptimizer optimizer;
        auto optimization_decision = optimizer.analyze_query(
            table_name,         // table name
            condition,          // WHERE clause
            input_rows,         // table size
            {}                  // available indexes (would be populated in Phase 6)
        );
        
        std::cout << "[Phase 4.3 ROUTING] Condition: " << condition << "\n";
        std::cout << "  Use index: " << (optimization_decision.use_index ? "YES" : "NO") << "\n";
        
        if (optimization_decision.use_index && !optimization_decision.primary_index.empty()) {
            // Phase 4.2 recommends indexed scan
            
            std::cout << "  Primary index: " << optimization_decision.primary_index << "\n";
            std::cout << "  Estimated speedup: " << optimization_decision.estimated_speedup << "x\n";
            
            // Extract column name from condition
            size_t op_pos = condition.find_first_of("<>=!~");
            if (op_pos != std::string::npos && op_pos > 0) {
                std::string col_name = condition.substr(0, op_pos);
                col_name.erase(col_name.find_last_not_of(" \t\n\r\f\v") + 1);
                
                // Extract operator and value
                size_t val_start = op_pos;
                std::string op;
                
                if (condition[op_pos] == '<' || condition[op_pos] == '>') {
                    if (op_pos + 1 < condition.length() && condition[op_pos + 1] == '=') {
                        op = condition.substr(op_pos, 2);
                        val_start = op_pos + 2;
                    } else {
                        op = condition.substr(op_pos, 1);
                        val_start = op_pos + 1;
                    }
                } else if (condition[op_pos] == '=') {
                    if (op_pos + 1 < condition.length() && condition[op_pos + 1] == '=') {
                        op = condition.substr(op_pos, 2);
                        val_start = op_pos + 2;
                    } else {
                        op = condition.substr(op_pos, 1);
                        val_start = op_pos + 1;
                    }
                }
                
                // Extract value
                std::string value_str = condition.substr(val_start);
                value_str.erase(0, value_str.find_first_not_of(" \t\n\r\f\v"));
                value_str.erase(value_str.find_last_not_of(" \t\n\r\f\v") + 1);
                
                // Format predicate for indexed scan
                std::string predicate = op + " " + value_str;
                
                // Attempt indexed scan
                std::cout << "  [INFO] Attempting indexed scan...\n";
                std::cout << "  [INFO] Index: " << optimization_decision.primary_index
                          << " Column: " << col_name << " Predicate: " << predicate << "\n";
                
                size_t indexed_result = execute_indexed_scan(
                    optimization_decision.primary_index,
                    col_name,
                    predicate,
                    ctx
                );
                
                if (indexed_result > 0) {
                    // Indexed scan succeeded
                    std::cout << "  [SUCCESS] Indexed scan returned " << indexed_result << " rows\n";
                    return indexed_result;
                }
            }
            
            std::cout << "  [FALLBACK] Index not available or scan failed, using full scan\n";
        } else {
            // Phase 4.2 recommends full scan
            std::cout << "  [INFO] Phase 4.2 recommends full scan (no index would help)\n";
            std::cout << "  [INFO] Reason: " << optimization_decision.reason << "\n";
        }
        
        // Fallback: full scan required
        // Return 0 to signal caller should use full scan
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "[Phase 4.3 ROUTING] Error: " << e.what() << "\n";
        return 0;  // Fallback to full scan on error
    }
}

}  // namespace lyradb
