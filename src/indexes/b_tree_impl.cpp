/**
 * @file b_tree_impl.cpp
 * @brief B-tree index runtime storage and integration
 * 
 * Phase 4.2: B-Tree Index Implementation
 * 
 * Provides:
 * - Single-column B-tree indexes
 * - Multi-column B-tree indexes using CompositeKey
 * - Range query support
 * - Index maintenance on INSERT/DELETE/DROP TABLE
 */

#include "lyradb/index_manager.h"
#include "lyradb/b_tree.h"
#include "lyradb/composite_key.h"
#include "lyradb/schema.h"
#include <memory>
#include <string>
#include <map>
#include <vector>

namespace lyradb {
namespace index {

/**
 * @class BTreeInstance
 * @brief Runtime storage for single-column B-tree indexes
 * 
 * Maintains B-tree instances for single-column range queries.
 */
class BTreeInstance {
public:
    using StringBTree = BTree<std::string, size_t>;
    
    std::unique_ptr<StringBTree> index;
    std::string table_name;
    std::string column_name;
    size_t row_count = 0;
    
    BTreeInstance(const std::string& table, const std::string& column)
        : table_name(table), column_name(column) {
        index = std::make_unique<StringBTree>();
    }
    
    // Make it moveable
    BTreeInstance(BTreeInstance&&) = default;
    BTreeInstance& operator=(BTreeInstance&&) = default;
    
    // Non-copyable
    BTreeInstance(const BTreeInstance&) = delete;
    BTreeInstance& operator=(const BTreeInstance&) = delete;
};

/**
 * @class CompositeBTreeInstance
 * @brief Runtime storage for multi-column B-tree indexes
 * 
 * Maintains B-tree instances for multi-column range queries.
 */
class CompositeBTreeInstance {
public:
    using CompositeBTree = BTree<CompositeKey, size_t>;
    
    std::unique_ptr<CompositeBTree> index;
    std::string table_name;
    std::vector<std::string> column_names;
    size_t row_count = 0;
    
    CompositeBTreeInstance(
        const std::string& table,
        const std::vector<std::string>& columns)
        : table_name(table), column_names(columns) {
        index = std::make_unique<CompositeBTree>();
    }
    
    // Make it moveable
    CompositeBTreeInstance(CompositeBTreeInstance&&) = default;
    CompositeBTreeInstance& operator=(CompositeBTreeInstance&&) = default;
    
    // Non-copyable
    CompositeBTreeInstance(const CompositeBTreeInstance&) = delete;
    CompositeBTreeInstance& operator=(const CompositeBTreeInstance&) = delete;
};

/**
 * @brief Global map of B-tree indexes
 * In production, this would be part of the database instance
 */
static std::map<std::string, std::shared_ptr<BTreeInstance>> g_btree_indexes;

/**
 * @brief Global map of composite B-tree indexes
 * In production, this would be part of the database instance
 */
static std::map<std::string, std::shared_ptr<CompositeBTreeInstance>> g_composite_btree_indexes;

/**
 * @brief Build a B-tree index from table data
 * @param index_name Index identifier
 * @param table_name Table being indexed
 * @param column_name Column being indexed
 * @param rows Table data
 * @param schema Table schema
 */
void build_btree_index(
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
    
    // Create B-tree instance
    auto index_inst = std::make_shared<BTreeInstance>(table_name, column_name);
    g_btree_indexes[index_name] = index_inst;
    
    // Build B-tree by inserting all rows
    for (size_t row_id = 0; row_id < rows.size(); ++row_id) {
        if (col_index < static_cast<int>(rows[row_id].size())) {
            const auto& key = rows[row_id][col_index];
            index_inst->index->insert(key, row_id);
        }
    }
    
    index_inst->row_count = rows.size();
}

/**
 * @brief Range search using a B-tree index
 * @param index_name Index identifier
 * @param min_key Minimum key (inclusive)
 * @param max_key Maximum key (inclusive)
 * @return Vector of row IDs in range
 */
std::vector<size_t> range_search_btree(
    const std::string& index_name,
    const std::string& min_key,
    const std::string& max_key) {
    
    auto it = g_btree_indexes.find(index_name);
    if (it == g_btree_indexes.end()) {
        return {};  // Index not found
    }
    
    if (!it->second) {
        return {};  // Null index
    }
    
    return it->second->index->range_search(min_key, max_key);
}

/**
 * @brief Look up exact key in B-tree index
 * @param index_name Index identifier
 * @param key Search key
 * @return Vector of row IDs matching the key
 */
std::vector<size_t> lookup_btree(
    const std::string& index_name,
    const std::string& key) {
    
    auto it = g_btree_indexes.find(index_name);
    if (it == g_btree_indexes.end()) {
        return {};  // Index not found
    }
    
    if (!it->second) {
        return {};  // Null index
    }
    
    return it->second->index->search(key);
}

/**
 * @brief Build a composite B-tree index from table data
 * @param index_name Index identifier
 * @param table_name Table being indexed
 * @param column_names Columns being indexed
 * @param rows Table data
 * @param schema Table schema
 */
void build_composite_btree_index(
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
    
    // Create composite B-tree instance
    auto index_inst = std::make_shared<CompositeBTreeInstance>(table_name, column_names);
    g_composite_btree_indexes[index_name] = index_inst;
    
    // Build B-tree by inserting all rows
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
        
        // Create CompositeKey and insert
        CompositeKey composite_key(key_values);
        index_inst->index->insert(composite_key, row_id);
    }
    
    index_inst->row_count = rows.size();
}

/**
 * @brief Range search using a composite B-tree index
 * @param index_name Index identifier
 * @param min_key Minimum composite key
 * @param max_key Maximum composite key
 * @return Vector of row IDs in range
 */
std::vector<size_t> range_search_composite_btree(
    const std::string& index_name,
    const std::vector<std::string>& min_key,
    const std::vector<std::string>& max_key) {
    
    auto it = g_composite_btree_indexes.find(index_name);
    if (it == g_composite_btree_indexes.end()) {
        return {};  // Index not found
    }
    
    if (!it->second) {
        return {};  // Null index
    }
    
    CompositeKey min_composite(min_key);
    CompositeKey max_composite(max_key);
    
    return it->second->index->range_search(min_composite, max_composite);
}

/**
 * @brief Lookup exact composite key in B-tree index
 * @param index_name Index identifier
 * @param key_values Composite key values
 * @return Vector of row IDs matching the key
 */
std::vector<size_t> lookup_composite_btree(
    const std::string& index_name,
    const std::vector<std::string>& key_values) {
    
    auto it = g_composite_btree_indexes.find(index_name);
    if (it == g_composite_btree_indexes.end()) {
        return {};  // Index not found
    }
    
    if (!it->second) {
        return {};  // Null index
    }
    
    CompositeKey composite_key(key_values);
    return it->second->index->search(composite_key);
}

/**
 * @brief Insert a row into all B-tree indexes on a table
 * @param table_name Table name
 * @param row_id Row identifier
 * @param row Row data
 * @param schema Table schema
 */
void update_btree_indexes(
    const std::string& table_name,
    size_t row_id,
    const std::vector<std::string>& row,
    const Schema& schema) {
    
    // Update single-column B-tree indexes
    for (auto& [index_name, index_inst_ptr] : g_btree_indexes) {
        if (!index_inst_ptr) continue;
        
        if (index_inst_ptr->table_name == table_name) {
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
 * @brief Insert a row into all composite B-tree indexes on a table
 * @param table_name Table name
 * @param row_id Row identifier
 * @param row Row data
 * @param schema Table schema
 */
void update_composite_btree_indexes(
    const std::string& table_name,
    size_t row_id,
    const std::vector<std::string>& row,
    const Schema& schema) {
    
    // Update composite B-tree indexes
    for (auto& [index_name, index_inst_ptr] : g_composite_btree_indexes) {
        if (!index_inst_ptr) continue;
        
        if (index_inst_ptr->table_name == table_name) {
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
 * @brief Clear all B-tree indexes for a table
 * @param table_name Table name
 */
void clear_btree_indexes(const std::string& table_name) {
    std::vector<std::string> to_remove;
    
    for (auto& [index_name, index_inst_ptr] : g_btree_indexes) {
        if (!index_inst_ptr) continue;
        
        if (index_inst_ptr->table_name == table_name) {
            to_remove.push_back(index_name);
        }
    }
    
    for (const auto& name : to_remove) {
        g_btree_indexes.erase(name);
    }
}

/**
 * @brief Clear all composite B-tree indexes for a table
 * @param table_name Table name
 */
void clear_composite_btree_indexes(const std::string& table_name) {
    std::vector<std::string> to_remove;
    
    for (auto& [index_name, index_inst_ptr] : g_composite_btree_indexes) {
        if (!index_inst_ptr) continue;
        
        if (index_inst_ptr->table_name == table_name) {
            to_remove.push_back(index_name);
        }
    }
    
    for (const auto& name : to_remove) {
        g_composite_btree_indexes.erase(name);
    }
}

} // namespace index
} // namespace lyradb
