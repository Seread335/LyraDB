#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include "database.h"

namespace lyradb {
namespace server {

/**
 * @brief REST API Server for LyraDB
 * Provides HTTP endpoints for database operations
 */
class RestServer {
public:
    /**
     * @brief Constructor
     * @param host Server host (default: 127.0.0.1)
     * @param port Server port (default: 8080)
     */
    RestServer(const std::string& host = "127.0.0.1", int port = 8080);
    
    /**
     * @brief Destructor
     */
    ~RestServer() = default;
    
    /**
     * @brief Start the REST server
     * @throws std::runtime_error if no database attached
     */
    void start();
    
    /**
     * @brief Stop the REST server
     */
    void stop();
    
    /**
     * @brief Attach a database instance to the server
     * @param db Shared pointer to Database instance
     */
    void attach_database(std::shared_ptr<Database> db);
    
    /**
     * @brief Handle POST /api/v1/query
     * Execute SQL query
     */
    std::string handle_query(const std::string& sql);
    
    /**
     * @brief Handle POST /api/v1/insert
     * Insert data into table
     */
    std::string handle_insert(const std::string& table_name, const std::string& json_data);
    
    /**
     * @brief Handle GET /api/v1/tables
     * List all tables
     */
    std::string handle_list_tables();
    
    /**
     * @brief Handle GET /api/v1/tables/{table}
     * Get table schema
     */
    std::string handle_get_table_schema(const std::string& table_name);
    
    /**
     * @brief Handle GET /api/v1/status
     * Get server status
     */
    std::string handle_status();
    
    /**
     * @brief Get server host
     */
    const std::string& get_host() const { return host_; }
    
    /**
     * @brief Get server port
     */
    int get_port() const { return port_; }
    
    /**
     * @brief Check if server is running
     */
    bool is_running() const { return is_running_; }
    
    /**
     * @brief Convert query result to JSON
     */
    std::string result_to_json(const std::vector<std::vector<std::string>>& rows,
                              const std::vector<std::string>& columns);
    
    /**
     * @brief Create JSON error response
     */
    std::string json_error(const std::string& message);
    
    /**
     * @brief Create JSON success response
     */
    std::string json_success(const std::string& message);

private:
    std::string host_;
    int port_;
    std::shared_ptr<Database> db_;
    bool is_running_ = false;
};

} // namespace server
} // namespace lyradb
