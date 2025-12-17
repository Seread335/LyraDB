#pragma once

#include "schema.h"
#include "data_types.h"
#include "expression_evaluator.h"
#include "aggregator.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace lyradb {

/**
 * @brief Engine for handling GROUP BY operations
 */
class GroupByEngine {
public:
    GroupByEngine() = default;
    ~GroupByEngine() = default;

    /**
     * @brief Convert table rows (vector<string>) to RowData (map<string, ExpressionValue>)
     */
    static std::vector<RowData> convert_rows_to_rowdata(
        const std::vector<std::vector<std::string>>& rows,
        const Schema& schema)
    {
        std::vector<RowData> result;
        
        for (const auto& row : rows) {
            RowData row_data;
            
            // Map each column name to its value
            for (size_t i = 0; i < schema.num_columns() && i < row.size(); ++i) {
                const auto& col = schema.get_column(i);
                const auto& value = row[i];
                
                // Convert string value to ExpressionValue based on column type
                if (value.empty() || value == "NULL") {
                    row_data[col.name] = nullptr;
                } else {
                    switch (col.type) {
                        case DataType::INT64:
                            try {
                                row_data[col.name] = static_cast<int64_t>(std::stoll(value));
                            } catch (...) {
                                row_data[col.name] = nullptr;
                            }
                            break;
                        case DataType::FLOAT64:
                            try {
                                row_data[col.name] = std::stod(value);
                            } catch (...) {
                                row_data[col.name] = nullptr;
                            }
                            break;
                        case DataType::VARCHAR:
                        case DataType::TEXT:
                            row_data[col.name] = value;
                            break;
                        case DataType::BOOLEAN:
                            {
                                bool bool_val = (value == "true" || value == "1" || value == "TRUE");
                                row_data[col.name] = bool_val;
                            }
                            break;
                        default:
                            row_data[col.name] = value;
                            break;
                    }
                }
            }
            
            result.push_back(row_data);
        }
        
        return result;
    }

    /**
     * @brief Create grouping key from row data based on GROUP BY expressions
     */
    static std::string create_grouping_key(
        const RowData& row,
        const std::vector<std::unique_ptr<expression::Expression>>& group_by_expressions)
    {
        std::string key;
        ExpressionEvaluator evaluator;
        
        for (size_t i = 0; i < group_by_expressions.size(); ++i) {
            if (i > 0) key += "|";
            
            auto value = evaluator.evaluate(group_by_expressions[i].get(), row);
            key += expression_value_to_string(value);
        }
        
        return key;
    }

    /**
     * @brief Perform GROUP BY aggregation
     * Returns a map from grouping key to aggregated row data
     */
    static std::map<std::string, RowData> group_and_aggregate(
        const std::vector<RowData>& rows,
        const std::vector<std::unique_ptr<expression::Expression>>& group_by_expressions,
        const std::vector<std::pair<std::string, int>>& aggregates) // (func_type, column_index)
    {
        std::map<std::string, RowData> groups;
        std::map<std::string, std::vector<RowData>> group_data;
        
        // Group rows
        for (const auto& row : rows) {
            std::string key = create_grouping_key(row, group_by_expressions);
            if (groups.find(key) == groups.end()) {
                groups[key] = row;  // Store first row of group for group by values
            }
            group_data[key].push_back(row);
        }
        
        // Apply aggregates to each group
        for (auto& [key, group_row] : groups) {
            const auto& group_rows = group_data[key];
            
            // TODO: Apply aggregates and add to group_row
            // This will depend on how aggregates are structured
        }
        
        return groups;
    }

private:
    /**
     * @brief Convert ExpressionValue to string
     */
    static std::string expression_value_to_string(const ExpressionValue& value)
    {
        if (std::holds_alternative<nullptr_t>(value)) {
            return "NULL";
        } else if (std::holds_alternative<int64_t>(value)) {
            return std::to_string(std::get<int64_t>(value));
        } else if (std::holds_alternative<double>(value)) {
            return std::to_string(std::get<double>(value));
        } else if (std::holds_alternative<std::string>(value)) {
            return std::get<std::string>(value);
        } else if (std::holds_alternative<bool>(value)) {
            return std::get<bool>(value) ? "true" : "false";
        }
        return "";
    }
};

} // namespace lyradb
