#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include "b_tree_index.h"
#include "hash_index.h"
#include "bitmap_index.h"

namespace lyradb {
namespace index {

/**
 * @brief Index type enumeration
 */
enum class IndexType {
    BTree,      // B-tree index for range queries
    Hash,       // Hash index for equality lookups
    Bitmap      // Bitmap index for low-cardinality columns
};

/**
 * @brief Index metadata and configuration
 */
struct IndexMetadata {
    std::string index_name;
    std::string table_name;
    std::string column_name;
    IndexType type;
    bool is_unique = false;
    size_t cardinality = 0;  // Number of distinct values
    
    IndexMetadata() = default;
    IndexMetadata(const std::string& name, const std::string& table,
                  const std::string& col, IndexType t)
        : index_name(name), table_name(table), column_name(col), type(t) {}
};

/**
 * @brief Index Manager - Central management of all database indexes
 * 
 * Manages lifecycle and coordination of multiple index types:
 * - B-tree indexes for efficient range queries
 * - Hash indexes for fast equality lookups
 * - Bitmap indexes for low-cardinality column filtering
 * 
 * Features:
 * - Automatic index selection based on column characteristics
 * - Index hints for query optimizer
 * - Statistics tracking for cardinality estimation
 * - Index persistence (metadata only in Phase 6)
 */
class IndexManager {
public:
    IndexManager() = default;
    ~IndexManager() = default;
    
    /**
     * @brief Create a B-tree index on a column
     * @param index_name Unique index identifier
     * @param table_name Table being indexed
     * @param column_name Column to index
     * @return True if created successfully
     */
    bool create_btree_index(const std::string& index_name,
                           const std::string& table_name,
                           const std::string& column_name) {
        if (indexes_.find(index_name) != indexes_.end()) {
            throw std::runtime_error("Index already exists: " + index_name);
        }
        
        // Create metadata
        IndexMetadata metadata(index_name, table_name, column_name, IndexType::BTree);
        indexes_metadata_[index_name] = metadata;
        
        // Create actual B-tree index (stored separately)
        // For Phase 6, we store metadata; actual index stored in query context
        return true;
    }
    
    /**
     * @brief Create a hash index on a column
     * @param index_name Unique index identifier
     * @param table_name Table being indexed
     * @param column_name Column to index
     * @return True if created successfully
     */
    bool create_hash_index(const std::string& index_name,
                          const std::string& table_name,
                          const std::string& column_name) {
        if (indexes_.find(index_name) != indexes_.end()) {
            throw std::runtime_error("Index already exists: " + index_name);
        }
        
        IndexMetadata metadata(index_name, table_name, column_name, IndexType::Hash);
        indexes_metadata_[index_name] = metadata;
        return true;
    }
    
    /**
     * @brief Create a bitmap index on a low-cardinality column
     * @param index_name Unique index identifier
     * @param table_name Table being indexed
     * @param column_name Column to index
     * @return True if created successfully
     */
    bool create_bitmap_index(const std::string& index_name,
                            const std::string& table_name,
                            const std::string& column_name) {
        if (indexes_.find(index_name) != indexes_.end()) {
            throw std::runtime_error("Index already exists: " + index_name);
        }
        
        IndexMetadata metadata(index_name, table_name, column_name, IndexType::Bitmap);
        indexes_metadata_[index_name] = metadata;
        return true;
    }
    
    /**
     * @brief Drop an index
     * @param index_name Name of index to drop
     * @return True if dropped successfully
     */
    bool drop_index(const std::string& index_name) {
        auto it = indexes_metadata_.find(index_name);
        if (it == indexes_metadata_.end()) {
            throw std::runtime_error("Index not found: " + index_name);
        }
        
        indexes_metadata_.erase(it);
        indexes_.erase(index_name);
        return true;
    }
    
    /**
     * @brief Check if index exists
     * @param index_name Index name
     * @return True if exists
     */
    bool index_exists(const std::string& index_name) const {
        return indexes_metadata_.find(index_name) != indexes_metadata_.end();
    }
    
    /**
     * @brief Get index metadata
     * @param index_name Index name
     * @return Index metadata
     */
    IndexMetadata get_index_metadata(const std::string& index_name) const {
        auto it = indexes_metadata_.find(index_name);
        if (it == indexes_metadata_.end()) {
            throw std::runtime_error("Index not found: " + index_name);
        }
        return it->second;
    }
    
    /**
     * @brief Get all indexes on a table
     * @param table_name Table name
     * @return Vector of index names
     */
    std::vector<std::string> get_indexes_on_table(const std::string& table_name) const {
        std::vector<std::string> result;
        
        for (const auto& pair : indexes_metadata_) {
            if (pair.second.table_name == table_name) {
                result.push_back(pair.first);
            }
        }
        
        return result;
    }
    
    /**
     * @brief Get all indexes on a column
     * @param table_name Table name
     * @param column_name Column name
     * @return Vector of index names
     */
    std::vector<std::string> get_indexes_on_column(const std::string& table_name,
                                                   const std::string& column_name) const {
        std::vector<std::string> result;
        
        for (const auto& pair : indexes_metadata_) {
            const auto& meta = pair.second;
            if (meta.table_name == table_name && meta.column_name == column_name) {
                result.push_back(pair.first);
            }
        }
        
        return result;
    }
    
    /**
     * @brief Update index statistics (cardinality)
     * @param index_name Index name
     * @param cardinality Number of distinct values
     */
    void update_statistics(const std::string& index_name, size_t cardinality) {
        auto it = indexes_metadata_.find(index_name);
        if (it == indexes_metadata_.end()) {
            throw std::runtime_error("Index not found: " + index_name);
        }
        
        it->second.cardinality = cardinality;
    }
    
    /**
     * @brief Select best index for a column based on characteristics
     * @param table_name Table name
     * @param column_name Column name
     * @param cardinality Estimated distinct values
     * @param query_type Type of query (eq, range, etc)
     * @return Recommended index type, or nullptr if none suitable
     */
    const char* recommend_index(const std::string& table_name,
                               const std::string& column_name,
                               size_t cardinality,
                               const std::string& query_type) const {
        // Heuristics for index selection
        if (cardinality < 100) {
            return "BITMAP";  // Low cardinality -> bitmap
        }
        
        if (query_type == "equality" && cardinality > 10000) {
            return "HASH";    // Equality on high cardinality -> hash
        }
        
        if (query_type == "range") {
            return "BTREE";   // Range queries -> B-tree
        }
        
        if (cardinality > 100 && cardinality < 10000) {
            return "BTREE";   // Medium cardinality -> B-tree
        }
        
        return nullptr;  // No recommendation
    }
    
    /**
     * @brief Get total number of indexes
     */
    size_t index_count() const {
        return indexes_metadata_.size();
    }
    
    /**
     * @brief Clear all indexes
     */
    void clear() {
        indexes_metadata_.clear();
        indexes_.clear();
    }
    
    /**
     * @brief Get all indexes
     * @return Vector of all index names
     */
    std::vector<std::string> get_all_indexes() const {
        std::vector<std::string> result;
        for (const auto& pair : indexes_metadata_) {
            result.push_back(pair.first);
        }
        return result;
    }

private:
    // Metadata for all indexes
    std::unordered_map<std::string, IndexMetadata> indexes_metadata_;
    
    // Index handles (placeholders for Phase 6)
    // In production, would store actual index instances
    std::unordered_map<std::string, void*> indexes_;
};

} // namespace index
} // namespace lyradb
