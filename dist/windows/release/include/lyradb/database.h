#pragma once

#include "schema.h"
#include "query_result.h"
#include "query_cache.h"
#include "index_manager.h"
#include <string>
#include <memory>
#include <map>
#include <vector>

namespace lyradb {

// Forward declarations
class Table;
class QueryExecutionEngine;

/**
 * @brief Main database entry point
 * Embeddable analytical database engine
 */
class Database {
public:
    /**
     * @brief Open or create a database file
     * @param path Path to .lyra database file
     */
    explicit Database(const std::string& path);
    
    ~Database();
    
    /**
     * @brief Create a new table
     */
    void create_table(const std::string& name, const Schema& schema);
    
    /**
     * @brief Get existing table
     */
    std::shared_ptr<Table> get_table(const std::string& name);
    
    /**
     * @brief Execute SQL query
     */
    std::unique_ptr<QueryResult> query(const std::string& sql);
    
    /**
     * @brief List all tables
     */
    std::vector<std::string> list_tables() const;
    
    /**
     * @brief Get table schema
     */
    Schema get_schema(const std::string& table_name) const;
    
    /**
     * @brief Close database
     */
    void close();
    
    /**
     * @brief Get query cache instance
     */
    QueryCache& get_cache() { return query_cache_; }
    
    /**
     * @brief Get index manager instance
     */
    index::IndexManager& get_index_manager() { return index_manager_; }
    
    /**
     * @brief Execute SQL directly without cache (for mutations)
     */
    std::unique_ptr<QueryResult> execute(const std::string& sql);
    
    // Properties
    const std::string& path() const { return path_; }
    bool is_open() const { return is_open_; }
    
private:
    std::string path_;
    bool is_open_ = false;
    std::map<std::string, std::shared_ptr<Table>> tables_;
    std::unique_ptr<QueryExecutionEngine> engine_;
    QueryCache query_cache_;  // LRU query result cache
    index::IndexManager index_manager_;  // Index management for Phase 4
};

} // namespace lyradb
