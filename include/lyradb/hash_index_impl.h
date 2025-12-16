#pragma once

#include <string>
#include <vector>
#include <memory>

namespace lyradb {

// Forward declarations
class Schema;

namespace index {

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
    const Schema& schema);

/**
 * @brief Look up rows using a hash index
 * @param index_name Index identifier
 * @param key Search key
 * @return Vector of row IDs matching the key
 */
std::vector<size_t> lookup_hash_index(
    const std::string& index_name,
    const std::string& key);

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
    const Schema& schema);

/**
 * @brief Remove rows from all indexes on a table
 * @param table_name Table name
 * @param row_ids Row identifiers to remove
 */
void remove_from_table_indexes(
    const std::string& table_name,
    const std::vector<size_t>& row_ids);

/**
 * @brief Clear all hash indexes for a table
 * @param table_name Table name
 */
void clear_table_indexes(const std::string& table_name);

/**
 * @brief Build a multi-column (composite) hash index
 * @param index_name Index identifier
 * @param table_name Table being indexed
 * @param column_names Columns being indexed
 * @param rows Table data
 * @param schema Table schema
 */
void build_composite_hash_index(
    const std::string& index_name,
    const std::string& table_name,
    const std::vector<std::string>& column_names,
    const std::vector<std::vector<std::string>>& rows,
    const Schema& schema);

/**
 * @brief Look up rows using a multi-column (composite) hash index
 * @param index_name Index identifier
 * @param key_values Column values for lookup
 * @return Vector of row IDs matching the composite key
 */
std::vector<size_t> lookup_composite_hash_index(
    const std::string& index_name,
    const std::vector<std::string>& key_values);

/**
 * @brief Insert a row into all composite indexes on a table
 * @param table_name Table name
 * @param row_id Row identifier
 * @param row Row data
 * @param schema Table schema
 */
void update_composite_table_indexes(
    const std::string& table_name,
    size_t row_id,
    const std::vector<std::string>& row,
    const Schema& schema);

/**
 * @brief Remove rows from all composite indexes on a table
 * @param table_name Table name
 * @param row_ids Row identifiers to remove
 */
void remove_from_composite_table_indexes(
    const std::string& table_name,
    const std::vector<size_t>& row_ids);

/**
 * @brief Clear all composite indexes for a table
 * @param table_name Table name
 */
void clear_composite_table_indexes(const std::string& table_name);

} // namespace index
} // namespace lyradb
