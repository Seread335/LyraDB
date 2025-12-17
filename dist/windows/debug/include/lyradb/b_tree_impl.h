/**
 * @file b_tree_impl.h
 * @brief B-tree index runtime interface
 * 
 * Phase 4.2: B-Tree Index Implementation
 * 
 * Public API for single and multi-column B-tree operations:
 * - Exact-match search
 * - Range search (>, <, >=, <=, BETWEEN)
 * - Index construction and maintenance
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

namespace lyradb {

// Forward declarations
class Schema;

namespace index {

/**
 * @brief Build a single-column B-tree index from table data
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
    const Schema& schema);

/**
 * @brief Range search using a B-tree index
 * @param index_name Index identifier
 * @param min_key Minimum key (inclusive)
 * @param max_key Maximum key (inclusive)
 * @return Vector of row IDs in range [min_key, max_key]
 */
std::vector<size_t> range_search_btree(
    const std::string& index_name,
    const std::string& min_key,
    const std::string& max_key);

/**
 * @brief Exact match search in B-tree index
 * @param index_name Index identifier
 * @param key Search key
 * @return Vector of row IDs matching the key
 */
std::vector<size_t> lookup_btree(
    const std::string& index_name,
    const std::string& key);

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
    const Schema& schema);

/**
 * @brief Range search using a composite B-tree index
 * @param index_name Index identifier
 * @param min_key Minimum composite key (vector of column values)
 * @param max_key Maximum composite key (vector of column values)
 * @return Vector of row IDs in range
 */
std::vector<size_t> range_search_composite_btree(
    const std::string& index_name,
    const std::vector<std::string>& min_key,
    const std::vector<std::string>& max_key);

/**
 * @brief Exact match search in composite B-tree index
 * @param index_name Index identifier
 * @param key_values Composite key values
 * @return Vector of row IDs matching the key
 */
std::vector<size_t> lookup_composite_btree(
    const std::string& index_name,
    const std::vector<std::string>& key_values);

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
    const Schema& schema);

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
    const Schema& schema);

/**
 * @brief Clear all B-tree indexes for a table
 * @param table_name Table name
 */
void clear_btree_indexes(const std::string& table_name);

/**
 * @brief Clear all composite B-tree indexes for a table
 * @param table_name Table name
 */
void clear_composite_btree_indexes(const std::string& table_name);

} // namespace index
} // namespace lyradb
