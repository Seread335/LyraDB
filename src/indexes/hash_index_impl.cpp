#include "lyradb/index_manager.h"
#include "lyradb/hash_index.h"
#include "lyradb/composite_key.h"
#include "lyradb/schema.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace lyradb {
namespace index {

/**
 * @class HashIndexInstance
 * @brief Runtime storage for hash index instances
 * 
 * Maintains actual hash index objects keyed by index name.
 * Phase 4.1 implementation for single-column and multi-column hash indexes.
 */
class HashIndexInstance {
public:
    using StringHashIndex = HashIndex<std::string, size_t>;
    
    std::unique_ptr<StringHashIndex> index;
    std::string table_name;
    std::string column_name;
    size_t row_count = 0;
    
    HashIndexInstance(const std::string& table, const std::string& column)
        : table_name(table), column_name(column) {
        index = std::make_unique<StringHashIndex>();
    }
    
    // Make it moveable
    HashIndexInstance(HashIndexInstance&&) = default;
    HashIndexInstance& operator=(HashIndexInstance&&) = default;
    
    // Non-copyable
    HashIndexInstance(const HashIndexInstance&) = delete;
    HashIndexInstance& operator=(const HashIndexInstance&) = delete;
};

/**
 * @class CompositeHashIndexInstance
 * @brief Runtime storage for multi-column hash indexes
 * 
 * Maintains composite key hash indexes for multi-column lookups.
 * Phase 4.1.2 implementation.
 */
class CompositeHashIndexInstance {
public:
    using CompositeHashIndex = HashIndex<CompositeKey, size_t>;
    
    std::unique_ptr<CompositeHashIndex> index;
    std::string table_name;
    std::vector<std::string> column_names;
    size_t row_count = 0;
    
    CompositeHashIndexInstance(
        const std::string& table,
        const std::vector<std::string>& columns)
        : table_name(table), column_names(columns) {
        index = std::make_unique<CompositeHashIndex>();
    }
    
    // Make it moveable
    CompositeHashIndexInstance(CompositeHashIndexInstance&&) = default;
    CompositeHashIndexInstance& operator=(CompositeHashIndexInstance&&) = default;
    
    // Non-copyable
    CompositeHashIndexInstance(const CompositeHashIndexInstance&) = delete;
    CompositeHashIndexInstance& operator=(const CompositeHashIndexInstance&) = delete;
};

/**
 * @brief Global map of hash indexes
 * In production, this would be part of the database instance
 */
static std::unordered_map<std::string, std::shared_ptr<HashIndexInstance>> g_hash_indexes;

/**
 * @brief Global map of composite hash indexes
 * In production, this would be part of the database instance
 */
static std::unordered_map<std::string, std::shared_ptr<CompositeHashIndexInstance>> g_composite_hash_indexes;

/**
 * @brief Build a hash index from table data
 * @param index_name Index identifier
 * @param table_name Table being indexed
 * @param column_name Column being indexed
 * @param rows Table data
 * @param schema Table schema
 */
void build_hash_index(
    const std::string& index_name,
    const std::string& table_name,
    const std::string& column_name,
    const std::vector<std::vector<std::string>>& rows,
    const Schema& schema) {
    
    // Find column index
    int col_index = -1;
    for (size_t i = 0; i < schema.num_columns(); ++i) {
        if (schema.get_column(i).name == column_name) {
            col_index = static_cast<int>(i);
            break;
        }
    }
    
    if (col_index < 0) {
        throw std::runtime_error("Column not found: " + column_name);
    }
    
    // Create index instance
    auto index_inst = std::make_shared<HashIndexInstance>(table_name, column_name);
    g_hash_indexes[index_name] = index_inst;
    
    // Build index by inserting all rows
    for (size_t row_id = 0; row_id < rows.size(); ++row_id) {
        if (col_index < static_cast<int>(rows[row_id].size())) {
            const auto& key = rows[row_id][col_index];
            index_inst->index->insert(key, row_id);
        }
    }
    
    index_inst->row_count = rows.size();
}

/**
 * @brief Look up rows using a hash index
 * @param index_name Index identifier
 * @param key Search key
 * @return Vector of row IDs matching the key
 */
std::vector<size_t> lookup_hash_index(
    const std::string& index_name,
    const std::string& key) {
    
    auto it = g_hash_indexes.find(index_name);
    if (it == g_hash_indexes.end()) {
        return {};  // Index not found
    }
    
    if (!it->second) {
        return {};  // Null index
    }
    
    return it->second->index->search(key);
}

/**
 * @brief Insert a row into all indexes on a table
 * @param table_name Table name
 * @param row_id Row identifier
 * @param row Row data
 * @param schema Table schema
 */
void update_table_indexes(
    const std::string& table_name,
    size_t row_id,
    const std::vector<std::string>& row,
    const Schema& schema) {
    
    // Find all hash indexes on this table
    for (auto& [index_name, index_inst_ptr] : g_hash_indexes) {
        if (!index_inst_ptr) continue;
        
        if (index_inst_ptr->table_name == table_name) {
            // Find the column in the row
            int col_index = -1;
            for (size_t i = 0; i < schema.num_columns(); ++i) {
                if (schema.get_column(i).name == index_inst_ptr->column_name) {
                    col_index = static_cast<int>(i);
                    break;
                }
            }
            
            if (col_index >= 0 && col_index < static_cast<int>(row.size())) {
                index_inst_ptr->index->insert(row[col_index], row_id);
            }
        }
    }
}

/**
 * @brief Remove rows from all indexes on a table
 * @param table_name Table name
 * @param row_ids Row identifiers to remove
 */
void remove_from_table_indexes(
    const std::string& table_name,
    const std::vector<size_t>& row_ids) {
    
    // Find all hash indexes on this table
    for (auto& [index_name, index_inst_ptr] : g_hash_indexes) {
        if (!index_inst_ptr) continue;
        
        if (index_inst_ptr->table_name == table_name) {
            for (size_t row_id : row_ids) {
                index_inst_ptr->index->remove(row_id);
            }
        }
    }
}

/**
 * @brief Clear all hash indexes for a table
 * @param table_name Table name
 */
void clear_table_indexes(const std::string& table_name) {
    std::vector<std::string> to_remove;
    
    for (auto& [index_name, index_inst_ptr] : g_hash_indexes) {
        if (!index_inst_ptr) continue;
        
        if (index_inst_ptr->table_name == table_name) {
            to_remove.push_back(index_name);
        }
    }
    
    for (const auto& name : to_remove) {
        g_hash_indexes.erase(name);
    }
}

/**
 * @brief Build a composite hash index from table data (Phase 4.1.2)
 * @param index_name Index identifier
 * @param table_name Table being indexed
 * @param column_names Columns being indexed (multiple columns)
 * @param rows Table data
 * @param schema Table schema
 */
void build_composite_hash_index(
    const std::string& index_name,
    const std::string& table_name,
    const std::vector<std::string>& column_names,
    const std::vector<std::vector<std::string>>& rows,
    const Schema& schema) {
    
    // Find column indices
    std::vector<int> col_indices;
    for (const auto& col_name : column_names) {
        int col_index = -1;
        for (size_t i = 0; i < schema.num_columns(); ++i) {
            if (schema.get_column(i).name == col_name) {
                col_index = static_cast<int>(i);
                break;
            }
        }
        
        if (col_index < 0) {
            throw std::runtime_error("Column not found: " + col_name);
        }
        
        col_indices.push_back(col_index);
    }
    
    // Create composite index instance
    auto index_inst = std::make_shared<CompositeHashIndexInstance>(table_name, column_names);
    g_composite_hash_indexes[index_name] = index_inst;
    
    // Build index by inserting all rows
    for (size_t row_id = 0; row_id < rows.size(); ++row_id) {
        // Create composite key from column values
        std::vector<std::string> key_values;
        
        for (int col_index : col_indices) {
            if (col_index < static_cast<int>(rows[row_id].size())) {
                key_values.push_back(rows[row_id][col_index]);
            } else {
                key_values.push_back("");
            }
        }
        
        // Create CompositeKey from values
        CompositeKey composite_key(key_values);
        index_inst->index->insert(composite_key, row_id);
    }
    
    index_inst->row_count = rows.size();
}

/**
 * @brief Look up rows using a composite hash index (Phase 4.1.2)
 * @param index_name Index identifier
 * @param key_values Search key values (one per indexed column)
 * @return Vector of row IDs matching the composite key
 */
std::vector<size_t> lookup_composite_hash_index(
    const std::string& index_name,
    const std::vector<std::string>& key_values) {
    
    auto it = g_composite_hash_indexes.find(index_name);
    if (it == g_composite_hash_indexes.end()) {
        return {};  // Index not found
    }
    
    if (!it->second) {
        return {};  // Null index
    }
    
    // Create composite key from values
    CompositeKey composite_key(key_values);
    return it->second->index->search(composite_key);
}

/**
 * @brief Update all composite indexes for a table on INSERT (Phase 4.1.2)
 * @param table_name Table name
 * @param row_id Row identifier
 * @param row Row data
 * @param schema Table schema
 */
void update_composite_table_indexes(
    const std::string& table_name,
    size_t row_id,
    const std::vector<std::string>& row,
    const Schema& schema) {
    
    // Find all composite indexes on this table
    for (auto& [index_name, index_inst_ptr] : g_composite_hash_indexes) {
        if (!index_inst_ptr) continue;
        
        if (index_inst_ptr->table_name == table_name) {
            // Find column indices for all indexed columns
            std::vector<int> col_indices;
            
            for (const auto& col_name : index_inst_ptr->column_names) {
                int col_index = -1;
                for (size_t i = 0; i < schema.num_columns(); ++i) {
                    if (schema.get_column(i).name == col_name) {
                        col_index = static_cast<int>(i);
                        break;
                    }
                }
                
                if (col_index >= 0) {
                    col_indices.push_back(col_index);
                }
            }
            
            // Create composite key from row values
            std::vector<std::string> key_values;
            for (int col_index : col_indices) {
                if (col_index < static_cast<int>(row.size())) {
                    key_values.push_back(row[col_index]);
                } else {
                    key_values.push_back("");
                }
            }
            
            CompositeKey composite_key(key_values);
            index_inst_ptr->index->insert(composite_key, row_id);
        }
    }
}

/**
 * @brief Remove rows from all composite indexes on a table (Phase 4.1.2)
 * @param table_name Table name
 * @param row_ids Row identifiers to remove
 */
void remove_from_composite_table_indexes(
    const std::string& table_name,
    const std::vector<size_t>& row_ids) {
    
    // Find all composite indexes on this table
    for (auto& [index_name, index_inst_ptr] : g_composite_hash_indexes) {
        if (!index_inst_ptr) continue;
        
        if (index_inst_ptr->table_name == table_name) {
            for (size_t row_id : row_ids) {
                index_inst_ptr->index->remove(row_id);
            }
        }
    }
}

/**
 * @brief Clear all composite indexes for a table (Phase 4.1.2)
 * @param table_name Table name
 */
void clear_composite_table_indexes(const std::string& table_name) {
    std::vector<std::string> to_remove;
    
    for (auto& [index_name, index_inst_ptr] : g_composite_hash_indexes) {
        if (!index_inst_ptr) continue;
        
        if (index_inst_ptr->table_name == table_name) {
            to_remove.push_back(index_name);
        }
    }
    
    for (const auto& name : to_remove) {
        g_composite_hash_indexes.erase(name);
    }
}

} // namespace index
} // namespace lyradb
