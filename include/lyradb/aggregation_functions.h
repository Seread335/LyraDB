#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <cstdint>

namespace lyradb {

/**
 * @brief Type for aggregation values (supports multiple types)
 */
using AggregationValue = std::variant<
    std::nullptr_t,      // NULL
    int64_t,            // Integer sum, count
    double,             // Average, decimal sum
    std::string         // String min/max
>;

/**
 * @brief Aggregation function types
 */
enum class AggregationFunction {
    COUNT,              // Count rows/values
    SUM,                // Sum numeric values
    AVG,                // Average numeric values
    MIN,                // Minimum value
    MAX,                // Maximum value
    COUNT_DISTINCT      // Count distinct values
};

/**
 * @brief Aggregation result container
 * Stores aggregate values for a single group
 */
struct AggregationResult {
    std::map<std::string, AggregationValue> values;  // aggregate_name -> result_value
    
    /**
     * @brief Set an aggregation result
     */
    void set_value(const std::string& name, const AggregationValue& value) {
        values[name] = value;
    }
    
    /**
     * @brief Get an aggregation result
     */
    AggregationValue get_value(const std::string& name) const {
        auto it = values.find(name);
        if (it != values.end()) {
            return it->second;
        }
        return nullptr;
    }
};

/**
 * @brief Aggregation accumulator for building results
 * Accumulates values for an aggregate function during grouping
 */
class AggregationAccumulator {
public:
    /**
     * @brief Initialize accumulator for a function
     */
    AggregationAccumulator(AggregationFunction func, const std::string& col_name = "")
        : func_(func), column_name_(col_name), count_(0), sum_(0.0), min_val_(), max_val_() {}
    
    /**
     * @brief Add a value to the accumulator
     * @param value String representation of value
     */
    void add_value(const std::string& value);
    
    /**
     * @brief Get the final aggregation result
     */
    AggregationValue get_result() const;
    
    /**
     * @brief Add distinct value (for COUNT DISTINCT)
     */
    void add_distinct(const std::string& value);
    
private:
    AggregationFunction func_;
    std::string column_name_;
    
    // Accumulators for different functions
    int64_t count_;
    double sum_;
    AggregationValue min_val_;
    AggregationValue max_val_;
    std::map<std::string, bool> distinct_values_;  // For COUNT DISTINCT
    
    /**
     * @brief Convert string to numeric value
     */
    bool try_parse_number(const std::string& str, double& out) const;
    
    /**
     * @brief Compare two aggregation values
     */
    bool compare_less(const AggregationValue& a, const AggregationValue& b) const;
};

/**
 * @brief Helper to identify if an expression is an aggregate function
 * @return true if expression is COUNT/SUM/AVG/MIN/MAX
 */
bool is_aggregate_function(const std::string& func_name);

/**
 * @brief Get aggregation function type from name
 */
AggregationFunction get_aggregation_type(const std::string& func_name);

}  // namespace lyradb
