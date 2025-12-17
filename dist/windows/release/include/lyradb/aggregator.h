#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace lyradb {

/**
 * @brief Aggregation function results
 */
struct AggregateResult {
    enum class Type {
        COUNT,
        SUM,
        AVG,
        MIN,
        MAX
    };
    
    Type type;
    double numeric_value = 0.0;
    int count = 0;
    
    AggregateResult(Type t) : type(t), numeric_value(0.0), count(0) {}
};

/**
 * @brief Aggregation function implementations
 */
class Aggregator {
public:
    Aggregator() = default;
    ~Aggregator() = default;
    
    /**
     * @brief Count non-NULL values
     */
    static int count(const std::vector<std::string>& values) {
        int result = 0;
        for (const auto& val : values) {
            if (!val.empty() && val != "NULL") {
                result++;
            }
        }
        return result;
    }
    
    /**
     * @brief Sum numeric values
     */
    static double sum(const std::vector<std::string>& values) {
        double result = 0.0;
        for (const auto& val : values) {
            if (!val.empty() && val != "NULL") {
                try {
                    result += std::stod(val);
                } catch (...) {
                    // Skip non-numeric values
                }
            }
        }
        return result;
    }
    
    /**
     * @brief Calculate average of numeric values
     */
    static double avg(const std::vector<std::string>& values) {
        int non_null_count = 0;
        double sum_val = 0.0;
        
        for (const auto& val : values) {
            if (!val.empty() && val != "NULL") {
                try {
                    sum_val += std::stod(val);
                    non_null_count++;
                } catch (...) {
                    // Skip non-numeric values
                }
            }
        }
        
        if (non_null_count == 0) return 0.0;
        return sum_val / non_null_count;
    }
    
    /**
     * @brief Find minimum numeric value
     */
    static double min_value(const std::vector<std::string>& values) {
        double min_val = std::numeric_limits<double>::max();
        bool found = false;
        
        for (const auto& val : values) {
            if (!val.empty() && val != "NULL") {
                try {
                    double num = std::stod(val);
                    if (!found || num < min_val) {
                        min_val = num;
                        found = true;
                    }
                } catch (...) {
                    // Skip non-numeric values
                }
            }
        }
        
        return found ? min_val : 0.0;
    }
    
    /**
     * @brief Find maximum numeric value
     */
    static double max_value(const std::vector<std::string>& values) {
        double max_val = std::numeric_limits<double>::lowest();
        bool found = false;
        
        for (const auto& val : values) {
            if (!val.empty() && val != "NULL") {
                try {
                    double num = std::stod(val);
                    if (!found || num > max_val) {
                        max_val = num;
                        found = true;
                    }
                } catch (...) {
                    // Skip non-numeric values
                }
            }
        }
        
        return found ? max_val : 0.0;
    }
    
    /**
     * @brief Convert aggregate result to string
     */
    static std::string to_string(double value) {
        // Check if value is a whole number
        if (value == std::floor(value)) {
            return std::to_string(static_cast<long long>(value));
        }
        return std::to_string(value);
    }
};

}  // namespace lyradb
