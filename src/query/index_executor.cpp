/**
 * @file index_executor.cpp
 * @brief Index Executor Implementation
 * 
 * Executes the optimization plans with real index operations:
 * - B-tree lookups for single value equality
 * - Range scans for inequality operators
 * - Set intersection for AND predicates
 * - Set union for OR predicates
 */

#include "lyradb/index_executor.h"
#include <chrono>
#include <cmath>
#include <iostream>
#include <algorithm>

namespace lyradb {
namespace integration {

IndexExecutor::IndexExecutor() {
}

IndexExecutor::~IndexExecutor() {
}

void IndexExecutor::register_index(
    const std::string& index_name,
    const std::string& column_name) {
    
    IndexInfo info;
    info.name = index_name;
    info.column_name = column_name;
    
    indexes_[index_name] = info;
}

void IndexExecutor::add_to_index(
    const std::string& index_name,
    const std::string& value,
    uint64_t row_id) {
    
    auto it = indexes_.find(index_name);
    if (it == indexes_.end()) {
        return;  // Index not found
    }
    
    // Add row ID to both the value mapping and the global row set
    it->second.value_to_rows[value].insert(row_id);
    it->second.row_ids.insert(row_id);
}

IndexResults IndexExecutor::lookup_value(
    const std::string& index_name,
    const std::string& value) {
    
    IndexResults result;
    result.success = false;
    result.execution_time_ms = 0.0;
    result.rows_examined = 0;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto it = indexes_.find(index_name);
    if (it == indexes_.end()) {
        result.error_message = "Index not found: " + index_name;
        return result;
    }
    
    // Look up value in the index
    auto val_it = it->second.value_to_rows.find(value);
    if (val_it != it->second.value_to_rows.end()) {
        result.row_ids = val_it->second;
        result.rows_examined = it->second.row_ids.size();  // All index entries
        result.success = true;
    } else {
        // Value not found, empty result set
        result.rows_examined = it->second.row_ids.size();
        result.success = true;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.execution_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    total_lookups_++;
    total_execution_time_ms_ += result.execution_time_ms;
    
    return result;
}

IndexResults IndexExecutor::range_scan(
    const std::string& index_name,
    const std::string& op,
    const std::string& value) {
    
    IndexResults result;
    result.success = false;
    result.execution_time_ms = 0.0;
    result.rows_examined = 0;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto it = indexes_.find(index_name);
    if (it == indexes_.end()) {
        result.error_message = "Index not found: " + index_name;
        return result;
    }
    
    result.rows_examined = it->second.row_ids.size();
    result.success = true;
    
    // Scan all values in the index and apply the operator
    for (const auto& [val, row_ids] : it->second.value_to_rows) {
        if (compare_values(val, value, op)) {
            result.row_ids.insert(row_ids.begin(), row_ids.end());
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.execution_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    total_scans_++;
    total_execution_time_ms_ += result.execution_time_ms;
    
    return result;
}

IndexResults IndexExecutor::intersect(
    const IndexResults& results1,
    const IndexResults& results2) {
    
    IndexResults result;
    result.success = true;
    result.rows_examined = results1.row_ids.size() + results2.row_ids.size();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Compute set intersection
    std::set_intersection(
        results1.row_ids.begin(), results1.row_ids.end(),
        results2.row_ids.begin(), results2.row_ids.end(),
        std::inserter(result.row_ids, result.row_ids.begin())
    );
    
    auto end = std::chrono::high_resolution_clock::now();
    result.execution_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    total_intersections_++;
    total_execution_time_ms_ += result.execution_time_ms;
    
    return result;
}

IndexResults IndexExecutor::unite(
    const IndexResults& results1,
    const IndexResults& results2) {
    
    IndexResults result;
    result.success = true;
    result.rows_examined = results1.row_ids.size() + results2.row_ids.size();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Compute set union
    std::set_union(
        results1.row_ids.begin(), results1.row_ids.end(),
        results2.row_ids.begin(), results2.row_ids.end(),
        std::inserter(result.row_ids, result.row_ids.begin())
    );
    
    auto end = std::chrono::high_resolution_clock::now();
    result.execution_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    total_unions_++;
    total_execution_time_ms_ += result.execution_time_ms;
    
    return result;
}

bool IndexExecutor::compare_values(
    const std::string& val1,
    const std::string& val2,
    const std::string& op) const {
    
    try {
        double v1 = std::stod(val1);
        double v2 = std::stod(val2);
        
        if (op == ">") return v1 > v2;
        if (op == "<") return v1 < v2;
        if (op == ">=") return v1 >= v2;
        if (op == "<=") return v1 <= v2;
        if (op == "==") return std::abs(v1 - v2) < 1e-9;
        if (op == "!=") return std::abs(v1 - v2) >= 1e-9;
    } catch (...) {
        // Fall back to string comparison
        if (op == ">") return val1 > val2;
        if (op == "<") return val1 < val2;
        if (op == ">=") return val1 >= val2;
        if (op == "<=") return val1 <= val2;
        if (op == "==") return val1 == val2;
        if (op == "!=") return val1 != val2;
    }
    
    return false;
}

std::string IndexExecutor::get_stats() const {
    std::string stats;
    stats += "Index Executor Statistics:\n";
    stats += "  Total lookups: " + std::to_string(total_lookups_) + "\n";
    stats += "  Total range scans: " + std::to_string(total_scans_) + "\n";
    stats += "  Total intersections: " + std::to_string(total_intersections_) + "\n";
    stats += "  Total unions: " + std::to_string(total_unions_) + "\n";
    stats += "  Total execution time: " + std::to_string(static_cast<int>(total_execution_time_ms_)) + " ms\n";
    return stats;
}

size_t IndexExecutor::get_index_size(const std::string& index_name) const {
    auto it = indexes_.find(index_name);
    if (it != indexes_.end()) {
        return it->second.row_ids.size();
    }
    return 0;
}

} // namespace integration
} // namespace lyradb
